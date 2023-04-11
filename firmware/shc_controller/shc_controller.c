/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2022..2023 Uwe Freese
*
* smarthomatic is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* smarthomatic is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with smarthomatic. If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "rfm12.h"
#include "../src_common/uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_dimmer.h"
#include "../src_common/msggrp_audio.h"
#include "../src_common/msggrp_display.h"
#include "../src_common/msggrp_gpio.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_controller.h"

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

#define LCD_BACKLIGHT_PORT  PORTC
#define LCD_BACKLIGHT_DDR   DDRC
#define LCD_BACKLIGHT_PIN   2

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"
#include "vlcd.h"

#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1
#define RFM_RESET_PIN_STATE 1

#define MENU_CURSOR_PORT   PORTA
#define MENU_CURSOR_PINREG PINA
#define MENU_UP_PIN        0
#define MENU_DOWN_PIN      1
#define MENU_LEFT_PIN      2
#define MENU_RIGHT_PIN     3

#define MENU_SET_PORT     PORTB
#define MENU_SET_PINREG   PINB
#define MENU_SET_PIN      1
#define MENU_CANCEL_PIN   2

#include "../src_common/util_watchdog_init.h"

#include "rgb_led.h"

typedef enum {
	KEY_NONE = 0,
	KEY_UP = 1,
	KEY_DOWN = 2,
	KEY_LEFT = 3,
	KEY_RIGHT = 4,
	KEY_SET = 5,
	KEY_CANCEL = 6
} MenuKeyEnum;

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t port_status_cycle;
uint16_t version_status_cycle;
uint16_t send_status_timeout = 5;

bool first_lcd_text = true;

int8_t  menu_item = -1; // -1 == Menu OFF
uint8_t menu_value[4] = { 0, 0, 0, 0 };
uint8_t menu_value_bak[4] = { 0, 0, 0, 0 };
uint8_t menu_item_name_length_max = 0;
uint8_t menu_item_max = 0;
char text[41];

#define TIMER1_TICK_DIVIDER 8 // 244 Hz / 8 = 32ms per animation_tick
uint8_t timer1_tick_divider = TIMER1_TICK_DIVIDER;

uint8_t max8(uint8_t a, uint8_t b)
{
	if (a > b)
		return a;
	else
		return b;
}

ISR (TIMER0_OVF_vect)
{
	timer1_tick_divider--;

	if (timer1_tick_divider == 0)
	{
		timer1_tick_divider = TIMER1_TICK_DIVIDER;
		animation_tick(true);
		animation_tick(false);
	}
}

void send_deviceinfo_status(void)
{
	inc_packetcounter();

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_CONTROLLER);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);

	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_CONTROLLER);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	rfm12_send_bufx();
}

void send_ack(uint32_t acksenderid, uint32_t ackpacketcounter, bool error)
{
	// any message can be used as ack, because they are the same anyway
	if (error)
	{
		UART_PUTS("Send error Ack\r\n");
		pkg_header_init_dimmer_coloranimation_ack();
	}

	inc_packetcounter();

	// set common fields
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);

	pkg_headerext_common_set_acksenderid(acksenderid);
	pkg_headerext_common_set_ackpacketcounter(ackpacketcounter);
	pkg_headerext_common_set_error(error);

	rfm12_send_bufx();
}

// Process a request to this device.
// React accordingly on the MessageType, MessageGroup and MessageID
// and send an Ack in any case. It may be an error ack if request is not supported.
void process_request(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	UART_PUTF("MessageGroupID:%u;", messagegroupid);

	if ((messagegroupid != MESSAGEGROUP_DISPLAY) && (messagegroupid != MESSAGEGROUP_AUDIO) && (messagegroupid != MESSAGEGROUP_DIMMER))
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}

	UART_PUTF("MessageID:%u;", messageid);

	if (((messagegroupid == MESSAGEGROUP_DISPLAY)
		&& (messageid != MESSAGEID_DISPLAY_TEXT))
		||
		((messagegroupid == MESSAGEGROUP_AUDIO)
		&& (messageid != MESSAGEID_AUDIO_TONE)
		&& (messageid != MESSAGEID_AUDIO_MELODY))
		||
		((messagegroupid == MESSAGEGROUP_DIMMER)
		&& (messageid != MESSAGEID_DIMMER_BRIGHTNESS)
		&& (messageid != MESSAGEID_DIMMER_COLOR)
		&& (messageid != MESSAGEID_DIMMER_COLORANIMATION)))
	{
		UART_PUTS("\r\nERR: Unsupported MessageID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}

	// "Set" or "SetGet" -> modify brightness/color/animation
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		if (messagegroupid == MESSAGEGROUP_DIMMER)
		{
			if (messageid == MESSAGEID_DIMMER_BRIGHTNESS)
			{
				rgb_led_user_brightness_factor = msg_dimmer_brightness_get_brightness();
				UART_PUTF("Brightness:%u;", rgb_led_user_brightness_factor);
				rgb_led_update_current_col();
			}
			else if (messageid == MESSAGEID_DIMMER_COLOR)
			{
				uint8_t color = msg_dimmer_color_get_color();
				UART_PUTF("Color:%u;", color);
				rgb_led_set_fixed_color(color);
			}
			else // MESSAGEID_DIMMER_COLORANIMATION
			{
				uint8_t i;

				cli();

				animation.repeat = msg_dimmer_coloranimation_get_repeat();
				animation.autoreverse = msg_dimmer_coloranimation_get_autoreverse();

				UART_PUTF2("Repeat:%u;AutoReverse:%u;", animation.repeat, animation.autoreverse);

				for (i = 0; i < ANIM_COL_ORIG_MAX; i++)
				{
					anim_time[i] = msg_dimmer_coloranimation_get_time(i);
					anim_colors_orig[i] = msg_dimmer_coloranimation_get_color(i);

					UART_PUTF2("Time[%u]:%u;", i, anim_time[i]);
					UART_PUTF2("Color[%u]:%u;", i, anim_colors_orig[i]);
				}

				init_animation(true);
				rgb_led_update_current_col();

				sei();
			}
		}
		else if (messagegroupid == MESSAGEGROUP_AUDIO)
		{
			if (messageid == MESSAGEID_AUDIO_TONE)
			{
				uint8_t tone = msg_audio_tone_get_tone();
				UART_PUTF("Tone:%u;", tone);
				speaker_set_fixed_tone(tone);
			}
			else // MESSAGEID_AUDIO_MELODY
			{
				uint8_t i;

				cli();

				melody.repeat = msg_audio_melody_get_repeat();
				melody.autoreverse = msg_audio_melody_get_autoreverse();

				UART_PUTF2("Repeat:%u;AutoReverse:%u;", melody.repeat, melody.autoreverse);

				for (i = 0; i < MELODY_TONE_ORIG_MAX; i++)
				{
					melody_time[i] = msg_audio_melody_get_time(i);
					melody_effect[i] = msg_audio_melody_get_effect(i);
					melody_tones_orig[i] = msg_audio_melody_get_tone(i);

					UART_PUTF2("Time[%u]:%u;", i, melody_time[i]);
					UART_PUTF2("Effect[%u]:%u;", i, melody_effect[i]);
					UART_PUTF2("Tone[%u]:%u;", i, melody_tones_orig[i]);
				}

				init_animation(false);
				speaker_update_current_tone();

				sei();
			}
		}
		else // MESSAGEGROUP_DISPLAY
		{
			if (messageid == MESSAGEID_DISPLAY_TEXT)
			{
				uint8_t y = msg_display_text_get_posy();
				uint8_t x = msg_display_text_get_posx();
				// Note: "Format" is not supported.
				msg_display_text_get_text(text);
				text[40] = 0; // set last character to zero byte in case the text in the message is 40 bytes long

				UART_PUTF("PosY:%u;", y);
				UART_PUTF("PosX:%u;", x);
				UART_PUTS("Text:");
				uart_putstr(text);

				if (first_lcd_text)
				{
					first_lcd_text = false;
					vlcd_clear();
				}

				vlcd_gotoyx(y, x);
				vlcd_puts(text);
			}
		}
	}

	UART_PUTS("\r\n");

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		if (messagegroupid == MESSAGEGROUP_DIMMER)
		{
			if (messageid == MESSAGEID_DIMMER_BRIGHTNESS)
			{
				pkg_header_init_dimmer_brightness_ack();
			}
			else if (messageid == MESSAGEID_DIMMER_COLOR)
			{
				pkg_header_init_dimmer_color_ack();
			}
			else // MESSAGEID_DIMMER_COLORANIMATION
			{
				pkg_header_init_dimmer_coloranimation_ack();
			}
		}
		else // MESSAGEGROUP_AUDIO
		{
			if (messageid == MESSAGEID_AUDIO_TONE)
			{
				pkg_header_init_audio_tone_ack();
			}
			else // MESSAGEID_AUDIO_MELODY
			{
				pkg_header_init_audio_melody_ack();
			}
		}

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		if (messagegroupid == MESSAGEGROUP_DIMMER)
		{
			if (messageid == MESSAGEID_DIMMER_BRIGHTNESS)
			{
				pkg_header_init_dimmer_brightness_ackstatus();
				msg_dimmer_brightness_set_brightness(rgb_led_user_brightness_factor);
			}
			else if (messageid == MESSAGEID_DIMMER_COLOR)
			{
				pkg_header_init_dimmer_color_ackstatus();
				msg_dimmer_color_set_color(anim_colors_orig[0]);
			}
			else // MESSAGEID_DIMMER_COLORANIMATION
			{
				uint8_t i;

				pkg_header_init_dimmer_coloranimation_ackstatus();

				for (i = 0; i < ANIM_COL_ORIG_MAX; i++)
				{
					msg_dimmer_coloranimation_set_color(i, anim_colors_orig[i]);
					msg_dimmer_coloranimation_set_time(i, anim_time[i]);
				}

				msg_dimmer_coloranimation_set_repeat(animation.repeat);
				msg_dimmer_coloranimation_set_autoreverse(animation.autoreverse);
			}
		}
		else // MESSAGEGROUP_AUDIO
		{
			if (messageid == MESSAGEID_AUDIO_TONE)
			{
				pkg_header_init_audio_tone_ackstatus();
				msg_audio_tone_set_tone(melody_tones_orig[0]);
			}
			else // MESSAGEID_AUDIO_MELODY
			{
				uint8_t i;

				pkg_header_init_audio_melody_ackstatus();

				for (i = 0; i < MELODY_TONE_ORIG_MAX; i++)
				{
					msg_audio_melody_set_time(i, melody_time[i]);
					msg_audio_melody_set_effect(i, melody_effect[i]);
					msg_audio_melody_set_tone(i, melody_tones_orig[i]);
				}

				msg_audio_melody_set_repeat(melody.repeat);
				msg_audio_melody_set_autoreverse(melody.autoreverse);
			}
		}

		UART_PUTS("Sending AckStatus\r\n");
	}

	send_ack(acksenderid, ackpacketcounter, false);
	send_status_timeout = 15;
}

// Check if incoming message is a legitimate request for this device.
// If not, ignore it.
void process_packet(uint8_t len)
{
	// Set packet content
	pkg_header_adjust_offset();

	// check SenderID
	uint32_t senderID = pkg_header_get_senderid();
	UART_PUTF("Packet Data: SenderID:%u;", senderID);

	if (senderID != 0)
	{
		UART_PUTS("\r\nERR: Illegal SenderID.\r\n");
		return;
	}

	// check PacketCounter
	// TODO: Reject if packet counter lower than remembered!!
	uint32_t packcnt = pkg_header_get_packetcounter();
	UART_PUTF("PacketCounter:%lu;", packcnt);

	if (0) // packcnt <= station_packetcounter ??
	{
		UART_PUTF("\r\nERR: Received PacketCounter < %lu.\r\n", station_packetcounter);
		return;
	}

	// write received counter
	station_packetcounter = packcnt;

	//e2p_powerswitch_set_basestationpacketcounter(station_packetcounter);

	// check MessageType
	MessageTypeEnum messagetype = pkg_header_get_messagetype();
	UART_PUTF("MessageType:%u;", messagetype);

	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_SET) && (messagetype != MESSAGETYPE_SETGET))
	{
		UART_PUTS("\r\nERR: Unsupported MessageType.\r\n");
		return;
	}

	// check device id
	uint16_t rcv_id = pkg_headerext_common_get_receiverid();

	UART_PUTF("ReceiverID:%u;", rcv_id);

	if (rcv_id != device_id)
	{
		UART_PUTS("\r\nWRN: DeviceID does not match.\r\n");
		return;
	}

	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();

	process_request(messagetype, messagegroupid, messageid);
}

// return first position of character in string
uint8_t findc(const char* s, char c, uint8_t occurrence)
{
	uint8_t pos = 0;

	while (*s)
	{
		if (*s == c)
		{
			occurrence--;

			if (occurrence == 0)
				return pos;
		}

		s++;
		pos++;
	}

	return 255;
}

// count occurrences of a character in a string
uint8_t findccount(const char* s, char c)
{
	uint8_t count = 0;

	while (*s)
	{
		if (*s == c)
			count++;

		s++;
	}

	return count;
}

void init_menu(void)
{
	uint8_t i, j, x;
	uint8_t len;
	uint8_t value_count;

	// remember value in case the user aborts the menu
	for (i = 0; i < 4; i++)
		menu_value_bak[i] = menu_value[i];

	menu_item = 0;

	// find max length of menu item names
	menu_item_name_length_max = 0;

	menu_item_max = 0;

	for (i = 0; i < 4; i++)
	{
		e2p_controller_get_menuoption(i, text);

		if (text[0] != 0)
		{
			menu_item_name_length_max = max8(menu_item_name_length_max, findc(text, '|', 1));
			menu_item_max = i;
		}
	}

	// print menu entries
	for (i = 0; i < 4; i++)
	{
		e2p_controller_get_menuoption(i, text);

		vlcd_gotoyx(8 + i, 0);

		if (text[0] != 0)
		{
			len = findc(text, '|', 1);
			value_count = findccount(text, '|');

			if (value_count > 0)
			{
				x = menu_item_name_length_max + 4;

				for (j = 0; j < menu_item_name_length_max - len; j++)
					vlcd_putc(' ');
				for (j = 0; j < len; j++)
					vlcd_putc(text[j]);
				VLCD_PUTS(": ");

				if (i == menu_item)
					vlcd_putc('[');
				else
					vlcd_putc(' ');

				j = findc(text, '|', menu_value[i] + 1) + 1;

				while (text[j] && (text[j] != '|'))
				{
					vlcd_putc(text[j]);
					j++;
					x++;
				}

				if (i == menu_item)
					vlcd_putc(']');
				else
					vlcd_putc(' ');

				while (x <= vlcd_chars_per_line)
				{
					vlcd_putc(' ');
					x++;
				}
			}
		}
	}
}

void leave_menu(bool save)
{
	uint8_t i;

	if (!save)
		for (i = 0; i < 4; i++)
			menu_value[i] = menu_value_bak[i];

	vlcd_clear_page(2);

	for (i = 0; i < 4; i++)
	{
		vlcd_gotoyx(9, 0);

		if (save)
			vlcd_puts(" *** Speichern ***");
		else
			vlcd_puts("  *** Abbruch ***");

		_delay_ms(500);

		vlcd_gotoyx(9, 0);
		vlcd_puts("                  ");
		_delay_ms(500);
	}

	menu_item = -1;
}

void menu_item_print(uint8_t item, bool selected)
{
	uint8_t j, x;

	e2p_controller_get_menuoption(item, text);
	x = menu_item_name_length_max + 2;
	vlcd_gotoyx(8 + item, x);

	if (selected)
		vlcd_putc('[');
	else
		vlcd_putc(' ');

	j = findc(text, '|', menu_value[item] + 1) + 1;

	while (text[j] && (text[j] != '|'))
	{
		vlcd_putc(text[j]);
		j++;
		x++;
	}

	if (selected)
		vlcd_putc(']');
	else
		vlcd_putc(' ');

	x += 2;

	while (x <= vlcd_chars_per_line)
	{
		vlcd_putc(' ');
		x++;
	}
}

void menu_up(void)
{
	if (menu_item > 0)
	{
		menu_item_print(menu_item, false);
		menu_item--;
		menu_item_print(menu_item, true);
	}
}

void menu_down(void)
{
	if (menu_item < menu_item_max)
	{
		menu_item_print(menu_item, false);
		menu_item++;
		menu_item_print(menu_item, true);
	}
}

void menu_right(void)
{
	e2p_controller_get_menuoption(menu_item, text);

	uint8_t value_count = findccount(text, '|');

	if (menu_value[menu_item] < value_count - 1)
	{
		menu_value[menu_item]++;
		menu_item_print(menu_item, true);
	}
}

void menu_left(void)
{
	if (menu_value[menu_item] > 0)
	{
		menu_value[menu_item]--;
		menu_item_print(menu_item, true);
	}
}

void handle_key(uint8_t skey)
{
	switch (skey)
	{
		case KEY_SET:
			if (menu_item == -1)
			{
				init_menu();
				vlcd_page(2);
			}
			else
			{
				leave_menu(true);
				vlcd_page(0);
			}
			break;
		case KEY_CANCEL:
			if (menu_item != -1)
			{
				leave_menu(false);
			}
			vlcd_page(0);
			break;
		case KEY_UP:
			if (menu_item == -1)
				vlcd_page(0);
			else
				menu_up();
			break;
		case KEY_DOWN:
			if (menu_item == -1)
				vlcd_page(1);
			else
				menu_down();
			break;
		case KEY_RIGHT:
			if (menu_item != -1)
				menu_right();
			break;
		case KEY_LEFT:
			if (menu_item != -1)
				menu_left();
			break;
		default:
			break;
	}
}

void lcd_backlight(bool on)
{
	if (on)
		sbi(LCD_BACKLIGHT_PORT, LCD_BACKLIGHT_PIN);
	else
		cbi(LCD_BACKLIGHT_PORT, LCD_BACKLIGHT_PIN);
}

void io_init(void)
{
	// disable JTAG and therefore enable pins PC2-PC4 as normal I/O pins
	MCUCR = (1<<JTD);
	MCUCR = (1<<JTD);

	util_init_led(&LED_DDR, &LED_PORT, LED_PIN);

	// LCD backlight
	sbi(LCD_BACKLIGHT_DDR, LCD_BACKLIGHT_PIN);

	// Turn on pull-up resistors on input pins for menu cursor keys
	sbi(MENU_CURSOR_PORT, MENU_UP_PIN);
	sbi(MENU_CURSOR_PORT, MENU_DOWN_PIN);
	sbi(MENU_CURSOR_PORT, MENU_LEFT_PIN);
	sbi(MENU_CURSOR_PORT, MENU_RIGHT_PIN);
	sbi(MENU_SET_PORT, MENU_SET_PIN);
	sbi(MENU_SET_PORT, MENU_CANCEL_PIN);
}

// Show colors shortly to tell user that power is connected (status LED may not be visible).
// In parallel, let status LED blink 3 times (as usual for SHC devices).
void startup_animation(void)
{
	struct rgb_color_t c;

	c = index2color(48);
	rgb_led_set_PWM(c);
	switch_led(true);
	_delay_ms(500);
	c = index2color(12);
	rgb_led_set_PWM(c);
	switch_led(false);
	_delay_ms(500);
	c = index2color(3);
	rgb_led_set_PWM(c);
	switch_led(true);
	_delay_ms(500);
	c = index2color(0);
	rgb_led_set_PWM(c);
	switch_led(false);
	_delay_ms(500);

	led_blink(500, 0, 1);
}

// make 20ms delay, call rfm12 tick and remember watchdog time
void rfm12_delay20(void)
{
	_delay_ms(20);
	rfm12_tick();
	rfm_watchdog_count(20);
}

MenuKeyEnum detect_key(void)
{
	if (!(MENU_CURSOR_PINREG & (1 << MENU_UP_PIN)))
		return KEY_UP;
	else if (!(MENU_CURSOR_PINREG & (1 << MENU_DOWN_PIN)))
		return KEY_DOWN;
	else if (!(MENU_CURSOR_PINREG & (1 << MENU_LEFT_PIN)))
		return KEY_LEFT;
	else if (!(MENU_CURSOR_PINREG & (1 << MENU_RIGHT_PIN)))
		return KEY_RIGHT;
	else if (!(MENU_SET_PINREG & (1 << MENU_SET_PIN)))
		return KEY_SET;
	else if (!(MENU_SET_PINREG & (1 << MENU_CANCEL_PIN)))
		return KEY_CANCEL;
	else
		return KEY_NONE;
}

MenuKeyEnum detect_key_debounced(void)
{
	uint8_t ms = 0;
	MenuKeyEnum key;
	MenuKeyEnum last_key = KEY_NONE;

	while (1)
	{
		key = detect_key();

		if (key == last_key)
			ms += 20;
		else
		{
			last_key = key;
			ms = 0;
		}

		if (ms >= 80)
			return key;

		rfm12_delay20();
	}
}

int main(void)
{
	uint8_t loop = 0;
	uint8_t i;
	uint16_t version_status_cycle;
	uint16_t version_status_cycle_counter;
	uint8_t lcd_type;
	MenuKeyEnum key;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	check_eeprom_compatibility(DEVICETYPE_CONTROLLER);
	lcd_type = e2p_controller_get_lcdtype();
	device_id = e2p_generic_get_deviceid();

	io_init();
	uart_init();

	if (lcd_type != LCDTYPE_NONE)
	{
		vlcd_init(lcd_type == LCDTYPE_4X40);
		lcd_backlight(true);
		vlcd_gotoyx(0, 0);
		VLCD_PUTS("smarthomatic");
		vlcd_gotoyx(1, 0);
		VLCD_PUTS("Controller");
		vlcd_gotoyx(2, 0);
		VLCD_PUTF4("v%u.%u.%u (%08lx)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
		vlcd_gotoyx(3, 0);
		VLCD_PUTF("DeviceID: %u", device_id);
		vlcd_gotoyx(4, 0);
		VLCD_PUTS("Page 2!");
	}

	// init button input
//	cbi(BUTTON_DDR, BUTTON_PIN);
//	sbi(BUTTON_PORT, BUTTON_PIN);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_controller_get_basestationpacketcounter();

	port_status_cycle = (uint16_t)e2p_controller_get_statuscycle() * 60;
	version_status_cycle = (uint16_t)(90000UL / port_status_cycle); // once every 25 hours
	version_status_cycle_counter = version_status_cycle - 1; // send right after startup

	rgb_led_brightness_factor = e2p_controller_get_brightnessfactor();

	osccal_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Controller v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2022..2023 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n", station_packetcounter);
	UART_PUTF ("Port status cycle: %us\r\n", port_status_cycle);
	UART_PUTF ("Version status cycle: %u * port status cycle\r\n", version_status_cycle);
	UART_PUTF ("E2P brightness factor: %u%%\r\n", rgb_led_brightness_factor);

	// init AES key
	e2p_generic_get_aeskey(aes_key);

	rfm_watchdog_init(device_id, e2p_controller_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN, RFM_RESET_PIN_STATE);
	rfm12_init();

	rgb_led_set_fixed_color(0);
	PWM_init();

 	startup_sound();
 	startup_animation();

	sei();

	while (42)
	{
		if (rfm12_rx_status() == STATUS_COMPLETE)
		{
			uint8_t len = rfm12_rx_len();

			rfm_watchdog_alive();

			if ((len == 0) || (len % 16 != 0))
			{
				UART_PUTF("Received garbage (%u bytes not multiple of 16): ", len);
				print_bytearray(bufx, len);
			}
			else // try to decrypt with all keys stored in EEPROM
			{
				memcpy(bufx, rfm12_rx_buffer(), len);

				UART_PUTS("Before decryption: ");
				print_bytearray(bufx, len);

				aes256_decrypt_cbc(bufx, len);

				UART_PUTS("Decrypted bytes: ");
				print_bytearray(bufx, len);

				if (!pkg_header_check_crc32(len))
				{
					UART_PUTS("Received garbage (CRC wrong after decryption).\r\n");
				}
				else
				{
					process_packet(len);
				}
			}

			// tell the implementation that the buffer can be reused for the next data.
			rfm12_rx_clear();
		}

		// tasks that are done every second
		if (loop == 50)
		{
			loop = 0;

			// when timeout active, flash LED
			/*if (cmd_timeout[0])
			{
				led_blink(10, 10, 1);
			}
			else
			{
				_delay_ms(20);
// 			}*/

			// send status from time to time
			if (!send_startup_reason(&mcusr_mirror))
			{
				send_status_timeout--;

				if (send_status_timeout == 0)
				{
//					send_status_timeout = port_status_cycle;
//					send_gpio_digitalporttimeout_status();
					led_blink(200, 0, 1);

					version_status_cycle_counter++;
				}
				else if (version_status_cycle_counter >= version_status_cycle)
				{
					version_status_cycle_counter = 0;
					send_deviceinfo_status();
					led_blink(200, 0, 1);
				}
			}
		}
		else
		{
			rfm12_delay20();
		}

		key = detect_key_debounced();

		if (key != KEY_NONE)
		{
			handle_key(key);

			while ((~MENU_CURSOR_PINREG & ((1 << MENU_UP_PIN) | (1 << MENU_DOWN_PIN) | (1 << MENU_LEFT_PIN) | (1 << MENU_RIGHT_PIN)))
				| (~MENU_SET_PINREG & ((1 << MENU_SET_PIN) | (1 << MENU_CANCEL_PIN))))
				rfm12_delay20();

			for (i = 0; i < 5; i++)
				rfm12_delay20();
		}

		loop++;
	}

	// never called
	// aes256_done(&aes_ctx);
}
