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
#include "../src_common/msggrp_controller.h"
#include "../src_common/msggrp_gpio.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_controller.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "../src_common/util_watchdog_init.h"
#include "version.h"
#include "vlcd.h"
#include "rgb_led.h"

#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1
#define RFM_RESET_PIN_STATE 1

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

#define LCD_BACKLIGHT_PORT  PORTC
#define LCD_BACKLIGHT_DDR   DDRC
#define LCD_BACKLIGHT_PIN   2

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

#define DELIVER_ACK_RETRIES 6 // real tries are one lower, because the last cycle is only for waiting for the Ack

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
uint32_t deliver_packetcounter;

uint32_t version_status_cycle = 0;
uint16_t menu_selection_status_cycle = 0;
uint16_t backlight_status_cycle = 0;
uint16_t deliver_ack_retries = 0;

uint8_t lcd_pages;
bool first_lcd_text = true;

uint8_t backlight_mode = 0;
uint8_t auto_backlight_time_sec = 0;
uint8_t auto_backlight_timeout = 0;

int8_t  menu_item = -1; // -1 == Menu OFF
uint8_t menu_value[4] = { 0, 0, 0, 0 };
uint8_t menu_value_tmp[4] = { 0, 0, 0, 0 };
uint8_t menu_item_name_length_max = 0;
uint8_t menu_item_max = 0;
uint8_t jump_back_sec = 0;
uint8_t jump_back_sec_menu = 0;
uint8_t jump_back_sec_page = 0;
SoundEnum sound = 0;
char text[121];

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

// Play back "OK"/"NOK" melody asynchronously in the background.
void melody_async(bool ok)
{
	if ((sound == SOUND_SAVECANCEL) || (sound == SOUND_KEY_SAVECANCEL))
	{
		cli();

		melody.repeat = 1;
		melody.autoreverse = false;

		melody_time[0]       = 4;
		melody_time[1]       = 4;
		melody_time[2]       = 4;
		melody_time[3]       = 4;
		melody_effect[0]     = 0;
		melody_effect[1]     = 0;
		melody_effect[2]     = 0;
		melody_effect[3]     = 0;
		melody_tones_orig[1] = 44;
		melody_tones_orig[3] = 0;

		if (ok)
		{
			melody_tones_orig[0] = 41;
			melody_tones_orig[2] = 49;
		}
		else
		{
			melody_tones_orig[0] = 49;
			melody_tones_orig[2] = 37;
		}

		init_animation(false);
		speaker_update_current_tone();

		sei();
	}
}

void vlcd_blink_text(uint8_t y, uint8_t x, char * s, bool error_beep)
{
	uint8_t i, j;
	uint8_t len = strlen(s);

	for (i = 0; i < 4; i++)
	{
		vlcd_gotoyx(y, x);

		if (error_beep)
			speaker_set_fixed_tone(13);

		vlcd_puts(s);

		for (j = 0; j < 25; j++)
			rfm12_delay20();

		vlcd_gotoyx(y, x);

		if (error_beep)
			speaker_set_fixed_tone(0);

		for (j = 0; j < len; j++)
			vlcd_putc(' ');

		for (j = 0; j < 25; j++)
			rfm12_delay20();
	}
}

void lcd_backlight(bool on)
{
	if (on)
		sbi(LCD_BACKLIGHT_PORT, LCD_BACKLIGHT_PIN);
	else
		cbi(LCD_BACKLIGHT_PORT, LCD_BACKLIGHT_PIN);
}

// count occurrences of a character in a string
uint8_t find_char_count(const char* s, char c)
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

void send_display_backlight_mode(void)
{
	inc_packetcounter();

	UART_PUTF2("Send Backlight: Mode %u, AutoTimeoutSec %u\r\n", backlight_mode, auto_backlight_time_sec);

	// Set packet content
	pkg_header_init_display_backlight_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_display_backlight_set_mode(backlight_mode);
	msg_display_backlight_set_autotimeoutsec(auto_backlight_time_sec);

	rfm12_send_bufx();
}

void send_controller_menuselection_status(bool deliver)
{
	uint8_t i;

	inc_packetcounter();

	// Set packet content
	if (deliver)
	{
		deliver_packetcounter = packetcounter;
		UART_PUTF2("Deliver MenuSelection (Retry %u, PacketCounter %lu):", DELIVER_ACK_RETRIES - deliver_ack_retries, deliver_packetcounter);
		pkg_header_init_controller_menuselection_deliver();
	}
	else
	{
		UART_PUTS("Send MenuSelection Status:");
		pkg_header_init_controller_menuselection_status();
	}

	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);

	for (i = 0; i <= menu_item_max; i++)
	{
		// when delivering, use the temporary values the user selected in the menu
		if (deliver)
			msg_controller_menuselection_set_index(i, menu_value_tmp[i] + 1);
		else
			msg_controller_menuselection_set_index(i, menu_value[i] + 1);

		UART_PUTF(" %u", menu_value[i] + 1);
	}

	UART_PUTS("\r\n");

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

	// An ACK doesn't contain message group and message id. So only check it for other message types.
	if (messagetype != MESSAGETYPE_ACK)
	{
		UART_PUTF("MessageGroupID:%u;", messagegroupid);

		if ((messagegroupid != MESSAGEGROUP_DISPLAY) && (messagegroupid != MESSAGEGROUP_AUDIO) && (messagegroupid != MESSAGEGROUP_DIMMER) && (messagegroupid != MESSAGEGROUP_CONTROLLER))
		{
			UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
			send_ack(acksenderid, ackpacketcounter, true);
			return;
		}

		UART_PUTF("MessageID:%u;", messageid);

		if (((messagegroupid == MESSAGEGROUP_DISPLAY)
			&& (messageid != MESSAGEID_DISPLAY_TEXT)
			&& (messageid != MESSAGEID_DISPLAY_BACKLIGHT))
			||
			((messagegroupid == MESSAGEGROUP_AUDIO)
			&& (messageid != MESSAGEID_AUDIO_TONE)
			&& (messageid != MESSAGEID_AUDIO_MELODY))
			||
			((messagegroupid == MESSAGEGROUP_CONTROLLER)
			&& (messageid != MESSAGEID_CONTROLLER_MENUSELECTION))
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
		else if (messagegroupid == MESSAGEGROUP_CONTROLLER)
		{
			if (messageid == MESSAGEID_CONTROLLER_MENUSELECTION)
			{
				uint8_t i, x, value_count;

				for (i = 0; i <= menu_item_max; i++)
				{
					x = msg_controller_menuselection_get_index(i);
					UART_PUTF2("Index[%u]:%u;", i, x);

					if (x != 0) { // 0 = not to be updated
						e2p_controller_get_menuoption(i, text);

						value_count = find_char_count(text, '|');

						if (x <= value_count) // within range of available options?
						{
							menu_value[i] = x - 1;
							e2p_controller_set_menuoptionindex(i, menu_value[i]);
						}
					}
				}
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
			else if (messageid == MESSAGEID_DISPLAY_BACKLIGHT)
			{
				backlight_mode = msg_display_backlight_get_mode();
				UART_PUTF("Mode:%u;", backlight_mode);
				lcd_backlight(backlight_mode == BACKLIGHTMODE_ON);

				uint8_t t = msg_display_backlight_get_autotimeoutsec();

				if (t != 0) {
					auto_backlight_time_sec = t;
					UART_PUTF("AutoBacklightTimeSec:%u;", t);
				}
			}
		}
	}

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
		else if (messagegroupid == MESSAGEGROUP_AUDIO)
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
		else if (messagegroupid == MESSAGEGROUP_CONTROLLER)
		{
			if (messageid == MESSAGEID_CONTROLLER_MENUSELECTION)
			{
				pkg_header_init_controller_menuselection_ack();
			}
		}
		else if (messagegroupid == MESSAGEGROUP_DISPLAY)
		{
			if (messageid == MESSAGEID_DISPLAY_TEXT)
			{
				pkg_header_init_display_text_ack();
			}
			else if (messageid == MESSAGEID_DISPLAY_BACKLIGHT)
			{
				pkg_header_init_display_backlight_ack();
			}
		}

		UART_PUTS("\r\nSending Ack\r\n");
		send_ack(acksenderid, ackpacketcounter, false);
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else if ((messagetype == MESSAGETYPE_GET) || (messagetype == MESSAGETYPE_SETGET))
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
		else if (messagegroupid == MESSAGEGROUP_CONTROLLER)
		{
			if (messageid == MESSAGEID_CONTROLLER_MENUSELECTION)
			{
				pkg_header_init_controller_menuselection_ackstatus();

				for (uint8_t i = 0; i <= menu_item_max; i++)
				{
					msg_controller_menuselection_set_index(i, menu_value[i] + 1);
				}
			}
		}
		else if (messagegroupid == MESSAGEGROUP_DISPLAY)
		{
			if (messageid == MESSAGEID_DISPLAY_TEXT)
			{
				// Irregularly answer with an ackstatus with empty data.
				// (We don't want to repeat the text.)
				pkg_header_init_display_text_ackstatus();
				// ... setting content skipped!
			}
			else if (messageid == MESSAGEID_DISPLAY_BACKLIGHT)
			{
				pkg_header_init_display_backlight_ackstatus();
				msg_display_backlight_set_mode(backlight_mode);
				msg_display_backlight_set_autotimeoutsec(auto_backlight_time_sec);
			}
		}
		else if (messagegroupid == MESSAGEGROUP_AUDIO)
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

		UART_PUTS("\r\nSending AckStatus\r\n");
		send_ack(acksenderid, ackpacketcounter, false);
	}
	// ACK for a previous "deliver" message?
	else if (messagetype == MESSAGETYPE_ACK)
	{
		ackpacketcounter = pkg_headerext_common_get_ackpacketcounter();
		uint8_t error = pkg_headerext_common_get_error();
		UART_PUTF("AckPacketCounter=%lu;", ackpacketcounter);
		UART_PUTF("Error=%u;\r\n", error);

		if (deliver_ack_retries == 0)
		{
			UART_PUTS("Ack received, but not expected. Ignoring!\r\n");
		}
		else if ((ackpacketcounter < deliver_packetcounter - DELIVER_ACK_RETRIES / 2) || (ackpacketcounter > deliver_packetcounter))
		{
			UART_PUTF("Ack received, but packetcounter is wrong (expected %lu). Ignoring!\r\n", deliver_packetcounter);
		}
		else if (error != 0)
		{
			UART_PUTS("Ack received, but error bit set. Ignoring!\r\n");
		}
		else
		{
			UART_PUTF("Deliver message successfully acknowledged after %u sec!\r\n", DELIVER_ACK_RETRIES - deliver_ack_retries);

			// store selected values in main array and e2p
			for (uint8_t i = 0; i < 4; i++)
			{
				menu_value[i] = menu_value_tmp[i];
				e2p_controller_set_menuoptionindex(i, menu_value[i]);
			}

			deliver_ack_retries = 0;
			melody_async(true);
			e2p_controller_get_menutextsuccess(text);
			vlcd_blink_text((VIRTUAL_LCD_PAGES - 1) * 4 + 2, (vlcd_chars_per_line - strlen(text)) / 2, text, false);
			vlcd_set_page(0);
		}
	}
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

	if ((messagetype != MESSAGETYPE_GET) && (messagetype != MESSAGETYPE_SET) && (messagetype != MESSAGETYPE_SETGET) && (messagetype != MESSAGETYPE_ACK))
	{
		UART_PUTS("\r\nERR: Unsupported MessageType.\r\n");
		return;
	}

	// check device id
	uint16_t rcv_id;

	if (messagetype == MESSAGETYPE_ACK)
	{
		rcv_id = pkg_headerext_common_get_acksenderid();
		UART_PUTF("AckSenderID:%u;", rcv_id);
	}
	else
	{
		rcv_id = pkg_headerext_common_get_receiverid();
		UART_PUTF("ReceiverID:%u;", rcv_id);
	}

	if (rcv_id != device_id)
	{
		UART_PUTS("\r\nDeviceID does not match.\r\n");
		return;
	}

	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();

	if (messagetype == MESSAGETYPE_ACK)
	{
		messagegroupid = 0;
		messageid = 0;
	}
	else
	{
		messagegroupid = pkg_headerext_common_get_messagegroupid();
		messageid = pkg_headerext_common_get_messageid();
	}

	process_request(messagetype, messagegroupid, messageid);
}

// return first position of character in string
uint8_t find_char(const char* s, char c, uint8_t occurrence)
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

void key_tone_ok(void)
{
	if ((sound == SOUND_KEY) || (sound == SOUND_KEY_SAVECANCEL))
	{
		speaker_set_fixed_tone(49);
		_delay_ms(10);
		speaker_set_fixed_tone(0);
	}
}

void key_tone_err(void)
{
	if ((sound == SOUND_KEY) || (sound == SOUND_KEY_SAVECANCEL))
	{
		speaker_set_fixed_tone(37);
		_delay_ms(10);
		speaker_set_fixed_tone(0);
	}
}

// find number of menu items and max length of menu value names
void init_menu_item_max(void)
{
	uint8_t i;
	menu_item_name_length_max = 0;
	menu_item_max = 0;

	// enforce that one item is available
	e2p_controller_get_menuoption(0, text);

	if (text[0] == 0)
	{
		strcpy(text, "Option1|Value1|Value2");
		e2p_controller_set_menuoption(0, text);
	}

	// search for last item
	for (i = 0; i < 4; i++)
	{
		e2p_controller_get_menuoption(i, text);

		if (text[0] != 0)
		{
			menu_item_name_length_max = max8(menu_item_name_length_max, find_char(text, '|', 1));
			menu_item_max = i;
		}
		else
			break;
	}
}

void init_menu(void)
{
	uint8_t i, j, x;
	uint8_t len;
	uint8_t value_count;

	// use temporary values, in case the user aborts the menu
	for (i = 0; i < 4; i++)
		menu_value_tmp[i] = menu_value[i];

	menu_item = 0;

	vlcd_clear_page(VIRTUAL_LCD_PAGES - 1);

	// print menu entries
	for (i = 0; i < 4; i++)
	{
		e2p_controller_get_menuoption(i, text);

		vlcd_gotoyx((VIRTUAL_LCD_PAGES - 1) * 4 + i, 0);

		if (text[0] != 0)
		{
			len = find_char(text, '|', 1);
			value_count = find_char_count(text, '|');

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

				j = find_char(text, '|', menu_value_tmp[i] + 1) + 1;

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

	jump_back_sec = jump_back_sec_menu;
}

void leave_menu(bool save)
{
	vlcd_clear_page(VIRTUAL_LCD_PAGES - 1);
	e2p_controller_get_menutextcancel(text);

	if (save)
	{
		// values are not saved to e2p and menu_value array now, but only after reception of deliver ack!
		key_tone_ok();
		deliver_ack_retries = DELIVER_ACK_RETRIES;
	}
	else
	{
		melody_async(false);

		vlcd_blink_text((VIRTUAL_LCD_PAGES - 1) * 4 + 1, (vlcd_chars_per_line - strlen(text)) / 2, text, false);
		vlcd_set_page(0);
	}

	menu_item = -1;
	jump_back_sec = 0;
}

void menu_item_print(uint8_t item, bool selected)
{
	uint8_t j, x;

	e2p_controller_get_menuoption(item, text);
	x = menu_item_name_length_max + 2;
	vlcd_gotoyx((VIRTUAL_LCD_PAGES - 1) * 4 + item, x);

	if (selected)
		vlcd_putc('[');
	else
		vlcd_putc(' ');

	j = find_char(text, '|', menu_value_tmp[item] + 1) + 1;

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
		key_tone_ok();
		menu_item_print(menu_item, false);
		menu_item--;
		menu_item_print(menu_item, true);
	}
	else
		key_tone_err();

	jump_back_sec = jump_back_sec_menu;
}

void menu_down(void)
{
	if (menu_item < menu_item_max)
	{
		key_tone_ok();
		menu_item_print(menu_item, false);
		menu_item++;
		menu_item_print(menu_item, true);
	}
	else
		key_tone_err();

	jump_back_sec = jump_back_sec_menu;
}

void menu_right(void)
{
	e2p_controller_get_menuoption(menu_item, text);

	uint8_t value_count = find_char_count(text, '|');

	if (menu_value_tmp[menu_item] < value_count - 1)
	{
		key_tone_ok();
		menu_value_tmp[menu_item]++;
		menu_item_print(menu_item, true);
	}
	else
		key_tone_err();

	jump_back_sec = jump_back_sec_menu;
}

void menu_left(void)
{
	if (menu_value_tmp[menu_item] > 0)
	{
		key_tone_ok();
		menu_value_tmp[menu_item]--;
		menu_item_print(menu_item, true);
	}
	else
		key_tone_err();

	jump_back_sec = jump_back_sec_menu;
}

void handle_key(uint8_t skey)
{
	switch (skey)
	{
		case KEY_SET:
			if (menu_item == -1)
			{
				key_tone_ok();
				init_menu();
				vlcd_set_page((VIRTUAL_LCD_PAGES - 1));
			}
			else
			{
				leave_menu(true);
			}
			break;
		case KEY_CANCEL:
			if (menu_item != -1)
			{
				leave_menu(false);
			}
			else if (vlcd_get_page() != 0)
			{
				key_tone_ok();
				vlcd_set_page(0);
			}
			else
			{
				key_tone_err();
			}
			jump_back_sec = 0;
			break;
		case KEY_UP:
			if (menu_item == -1)
			{
				if (vlcd_get_page() > 0)
				{
					key_tone_ok();
					vlcd_set_page(vlcd_get_page() - 1);
				}
				else
				{
					key_tone_err();
				}
				jump_back_sec = 0;
			}
			else
				menu_up();
			break;
		case KEY_DOWN:
			if (menu_item == -1)
			{
				if (vlcd_get_page() < lcd_pages - 1)
				{
					key_tone_ok();
					vlcd_set_page(vlcd_get_page() + 1);
				}
				else
				{
					key_tone_err();
				}
				jump_back_sec = jump_back_sec_page;
			}
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

		if (key == KEY_NONE)
			return key;

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

	uint32_t version_status_cycle_counter;
	uint16_t menu_selection_status_cycle_counter;
	uint16_t backlight_status_cycle_counter;

	uint8_t lcd_type;

	MenuKeyEnum key;

	// delay 1s to avoid further communication with UART or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	check_eeprom_compatibility(DEVICETYPE_CONTROLLER);
	lcd_type = e2p_controller_get_lcdtype();
	device_id = e2p_generic_get_deviceid();
	jump_back_sec_menu = e2p_controller_get_pagejumpbackseconds();
	jump_back_sec_page = e2p_controller_get_menujumpbackseconds();
	backlight_mode = e2p_controller_get_backlightmode();
	auto_backlight_time_sec = e2p_controller_get_autobacklighttimesec();
	sound = e2p_controller_get_sound();
	lcd_pages = e2p_controller_get_lcdpages();
	init_menu_item_max();

	io_init();
	uart_init();

	if (lcd_type != LCDTYPE_NONE)
	{
		vlcd_init(lcd_type == LCDTYPE_4X40);

		if ((backlight_mode == BACKLIGHTMODE_ON) || (backlight_mode == BACKLIGHTMODE_AUTO))
			lcd_backlight(true);

		if (backlight_mode == BACKLIGHTMODE_AUTO)
		{
			auto_backlight_timeout = auto_backlight_time_sec;
		}

		vlcd_gotoyx(0, 0);
		VLCD_PUTS("smarthomatic");
		vlcd_gotoyx(1, 0);
		VLCD_PUTS("Controller");
		vlcd_gotoyx(2, 0);
		VLCD_PUTF4("v%u.%u.%u (%08lx)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
		vlcd_gotoyx(3, 0);
		VLCD_PUTF("DeviceID: %u", device_id);
	}

	for (i = 0; i < 4; i++)
		menu_value[i] = e2p_controller_get_menuoptionindex(i);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_controller_get_basestationpacketcounter();

	version_status_cycle        = (uint32_t)(90000UL); // once every 25 hours
	menu_selection_status_cycle = (uint16_t)e2p_controller_get_menuselectionstatuscycle() * 60;
	backlight_status_cycle      = (uint16_t)e2p_controller_get_backlightstatuscycle() * 60;

	// send all status messages right after startup
	version_status_cycle_counter        = version_status_cycle - 5;
	menu_selection_status_cycle_counter = menu_selection_status_cycle == 0 ? 0 : menu_selection_status_cycle - 10;
	backlight_status_cycle_counter      = backlight_status_cycle == 0 ? 0 : backlight_status_cycle - 15;

	rgb_led_brightness_factor = e2p_controller_get_brightnessfactor();

	osccal_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Controller v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2022..2023 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n", station_packetcounter);
	UART_PUTF ("Version status cycle: %lus\r\n", version_status_cycle);
	UART_PUTF ("Menu selection status cycle: %us\r\n", menu_selection_status_cycle);
	UART_PUTF ("Backlight status cycle: %us\r\n", backlight_status_cycle);
	UART_PUTF ("E2P brightness factor: %u%%\r\n", rgb_led_brightness_factor);
	UART_PUTF ("LCD type: %u\r\n", lcd_type);
	UART_PUTF ("Page jump back seconds: %u\r\n", jump_back_sec_page);
	UART_PUTF ("Menu jump back seconds: %u\r\n", jump_back_sec_menu);
	UART_PUTF ("Backlight mode: %u\r\n", backlight_mode);
	UART_PUTF ("Auto backlight time seconds: %u\r\n", auto_backlight_time_sec);
	UART_PUTF ("Sound: %u\r\n", sound);

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

				// Set the (len+1)th byte to 0, because the last packet content (from the last byte)
				// may be smaller than 8 bits and is read per byte in decode_data.
				bufx[len] = 0;

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
		if ((deliver_ack_retries == DELIVER_ACK_RETRIES) || (loop == 50))
		{
			loop = 0;

			if (deliver_ack_retries > 0)
			{
				deliver_ack_retries--;

				if (deliver_ack_retries > 0)
				{
					e2p_controller_get_menutextsendoptions(text);
					vlcd_gotoyx((VIRTUAL_LCD_PAGES - 1) * 4 + 1, (vlcd_chars_per_line - strlen(text)) / 2);
					vlcd_puts(text);
					vlcd_gotoyx((VIRTUAL_LCD_PAGES - 1) * 4 + 2, vlcd_chars_per_line / 2);
					VLCD_PUTF("%u", DELIVER_ACK_RETRIES - deliver_ack_retries);
					send_controller_menuselection_status(true);
					rfm12_send_wait_led();
				}
				else
				{
					e2p_controller_get_menutextfailed(text);
					vlcd_blink_text((VIRTUAL_LCD_PAGES - 1) * 4 + 2, (vlcd_chars_per_line - strlen(text)) / 2, text, true);
					vlcd_set_page(0);
				}
			}
			else
			{
				if (jump_back_sec > 0)
				{
					jump_back_sec--;

					if (jump_back_sec == 0)
					{
						if (menu_item != -1)
							leave_menu(false);
						vlcd_set_page(0);
					}
				}

				version_status_cycle_counter++;

				if (menu_selection_status_cycle)
					menu_selection_status_cycle_counter++;

				if (backlight_status_cycle)
					backlight_status_cycle_counter++;

				// send status from time to time
				if (!send_startup_reason(&mcusr_mirror))
				{
					//UART_PUTF2("Status counters: version %lu/%lu, ", version_status_cycle_counter, version_status_cycle);
					//UART_PUTF2("menu selection %u/%u, ", menu_selection_status_cycle_counter, menu_selection_status_cycle);
					//UART_PUTF2("backlight %u/%u\r\n", backlight_status_cycle_counter, backlight_status_cycle);

					if (version_status_cycle_counter >= version_status_cycle)
					{
						version_status_cycle_counter = 0;
						send_deviceinfo_status();
						rfm12_send_wait_led();
					}
					else if (menu_selection_status_cycle && (menu_selection_status_cycle_counter >= menu_selection_status_cycle))
					{
						menu_selection_status_cycle_counter = 0;
						send_controller_menuselection_status(false);
						rfm12_send_wait_led();
					}
					else if (backlight_status_cycle && (backlight_status_cycle_counter >= backlight_status_cycle))
					{
						backlight_status_cycle_counter = 0;
						send_display_backlight_mode();
						rfm12_send_wait_led();
					}
				}
			}

			if (backlight_mode == BACKLIGHTMODE_AUTO) {
				if (auto_backlight_timeout > 0)
				{
					auto_backlight_timeout--;

					if (auto_backlight_timeout == 0)
					{
						lcd_backlight(false);
					}
				}
			}
		}
		else
		{
			rfm12_delay20();
		}

		// Key handling is enabled only when not waiting for a deliver ack from basestation
		if (deliver_ack_retries == 0)
		{
			key = detect_key_debounced();

			if (key != KEY_NONE)
			{
				if (backlight_mode == BACKLIGHTMODE_AUTO) {
					if (auto_backlight_timeout == 0)
						lcd_backlight(true);

					auto_backlight_timeout = auto_backlight_time_sec;
				}

				handle_key(key);

				while ((~MENU_CURSOR_PINREG & ((1 << MENU_UP_PIN) | (1 << MENU_DOWN_PIN) | (1 << MENU_LEFT_PIN) | (1 << MENU_RIGHT_PIN)))
					| (~MENU_SET_PINREG & ((1 << MENU_SET_PIN) | (1 << MENU_CANCEL_PIN))))
					rfm12_delay20();

				for (i = 0; i < 5; i++)
					rfm12_delay20();
			}
		}

		loop++;
	}

	// never called
	// aes256_done(&aes_ctx);
}
