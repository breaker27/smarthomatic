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

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1
#define RFM_RESET_PIN_STATE 1

#include "../src_common/util_watchdog_init.h"

#include "rgb_led.h"

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t port_status_cycle;
uint16_t version_status_cycle;
uint16_t send_status_timeout = 5;

char text[41];

#define TIMER1_TICK_DIVIDER 8 // 244 Hz / 8 = 32ms per animation_tick
uint8_t timer1_tick_divider = TIMER1_TICK_DIVIDER;

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

void io_init(void)
{
	// disable JTAG and therefore enable pins PC2-PC4 as normal I/O pins
	MCUCR = (1<<JTD);
	MCUCR = (1<<JTD);

	util_init_led(&LED_DDR, &LED_PORT, LED_PIN);
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

int main(void)
{
	uint8_t loop = 0;
	uint8_t i;
	uint8_t button = 0;
	uint8_t button_old = 0;
	uint8_t button_debounce = 0;
	uint16_t version_status_cycle;
	uint16_t version_status_cycle_counter;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	io_init();

	check_eeprom_compatibility(DEVICETYPE_CONTROLLER);

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

	// read device id
	device_id = e2p_generic_get_deviceid();

	rgb_led_brightness_factor = e2p_controller_get_brightnessfactor();

	osccal_init();

	uart_init();

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
			_delay_ms(20);
		}

		rfm_watchdog_count(20);

		rfm12_tick();

		//TODO: Button handling.

		loop++;
	}

	// never called
	// aes256_done(&aes_ctx);
}
