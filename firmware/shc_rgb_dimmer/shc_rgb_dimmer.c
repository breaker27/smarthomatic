/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2014 Uwe Freese
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
#include "uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_dimmer.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_rgbdimmer.h"

#include "aes256.h"
#include "util.h"
#include "version.h"

#define RGBLED_DDR DDRD
#define RGBLED_PORT PORTD
#define RGBLED_PINPORT PIND

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent?
#define SEND_VERSION_STATUS_CYCLE 50 // send version status x times less than switch status (~once per day)

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t send_status_timeout = 5;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

uint8_t brightness_factor;

#define RED_PIN 6
#define GRN_PIN 5
#define BLU_PIN 1

#define RED_DDR DDRD
#define GRN_DDR DDRD
#define BLU_DDR DDRB

// For the given 5 bit animation time, these are the lengths in timer 2 cycles.
// The input value x means 0.05s * 1.3 ^ x and covers 30ms to 170s. Each timer cycle is 32.768ms.
// Therefore, the values are: round((0.05s * 1.3 ^ x) / 0.032768s).
// Animation time 0 is animation OFF, so there are 31 defined animation times.
const uint16_t anim_cycles[31] = {1, 2, 3, 4, 6, 8, 10, 12, 16, 21, 27, 36, 46, 60,
                                  78, 102, 132, 172, 223, 290, 377, 490, 637, 828,
                                  1077, 1400, 1820, 2366, 3075, 3998, 5197};

struct rgb_color_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct rgb_color_t anim_col[11]; // The last active color (index 0) + 10 new colors used for the animation (index 1-10).
uint8_t anim_time[10];           // The times used for animate blending between two colors.
                                 // 0 indicates the last color used.
uint8_t anim_col_i[10];          // The 10 (indexed) colors used for the animation.
uint8_t anim_repeat;             // Number of repeats, 0 = endless.
bool anim_autoreverse;           // Play back animation in reverse order when finished.
uint16_t anim_len = 0;           // length of animation of current color to next color.
                                 // max. anim. time is ~ 340s = 10600 steps à 32ms
uint16_t anim_pos = 0;           // Position in the current animation.
uint8_t anim_col_pos = 0;        // Index of currently animated color.

// Timer0 (8 Bit) and Timer1 (10 Bit in 8 Bit mode) are used for the PWM output for the LEDs.
// Read for more information about PWM:
// http://www.protostack.com/blog/2011/06/atmega168a-pulse-width-modulation-pwm/
// http://extremeelectronics.co.in/avr-tutorials/pwm-signal-generation-by-using-avr-timers-part-ii/
void PWM_init(void)
{
	// Enable output pins
	RED_DDR |= (1 << RED_PIN);
	GRN_DDR |= (1 << GRN_PIN);
	BLU_DDR |= (1 << BLU_PIN);

	// OC0A (Red LED): Fast PWM, 8 Bit, TOP = 0xFF = 255, non inverting output
	// OC0B (Green LED): Fast PWM, 8 Bit, TOP = 0xFF = 255, non inverting output
	TCCR0A = (1 << WGM00) | (1 << COM0A1) | (1 << COM0B1);

	// OC1A (Blue LED): Fast PWM, 8 Bit, TOP = 0xFF = 255, non inverting output
	TCCR1A = (1 << WGM10) | (1 << COM1A1);

	// Clock source for both timers = I/O clock, no prescaler
	TCCR0B = (1 << CS00);
	TCCR1B = (1 << CS10);
}

// Timer2 (8 Bit) is used for accurate counting of the animation time.
void timer2_init(void)
{
	// Clock source = I/O clock, 1/1024 prescaler
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);

	// Timer/Counter2 Overflow Interrupt Enable
	TIMSK2 = (1 << TOIE2);
}

void set_PWM(struct rgb_color_t color)
{
	OCR0A = (uint16_t)color.r * brightness_factor / 100;
	OCR0B = (uint16_t)color.g * brightness_factor / 100;
	OCR1A = (uint16_t)color.b * brightness_factor / 100;
}

// Calculate an RGB value out of the index color.
// The color palette is 6 bit with 2 bits per color (same as EGA).
// Bit 1+0 = blue
// Bit 3+2 = green
// Bit 5+4 = red
// The brightness per color can be:
// 0 -> 0
// 1 -> 85
// 2 -> 170
// 3 -> 255
struct rgb_color_t index2color(uint8_t color)
{
	struct rgb_color_t res;
	
	res.r = ((color & 0b110000) >> 4) * 85;
	res.g = ((color & 0b001100) >> 2) * 85;
	res.b = ((color & 0b000011) >> 0) * 85;
	
	// UART_PUTF4("Index %d to color -> %d,%d,%d\r\n", color, res.r, res.g, res.b);
	
	return res;
}

// Calculate the color that has to be shown according to the animation settings and counter.
struct rgb_color_t calc_pwm_color(void)
{
	struct rgb_color_t res;
	
	res.r = (uint8_t)((uint32_t)anim_col[anim_col_pos].r * (anim_len - anim_pos) / anim_len
		+ (uint32_t)anim_col[anim_col_pos + 1].r * anim_pos / anim_len);
	res.g = (uint8_t)((uint32_t)anim_col[anim_col_pos].g * (anim_len - anim_pos) / anim_len
		+ (uint32_t)anim_col[anim_col_pos + 1].g * anim_pos / anim_len);
	res.b = (uint8_t)((uint32_t)anim_col[anim_col_pos].b * (anim_len - anim_pos) / anim_len
		+ (uint32_t)anim_col[anim_col_pos + 1].b * anim_pos / anim_len);

	//UART_PUTF("anim_col_pos %d, ", anim_col_pos);
	//UART_PUTF3("Animation PWM color %d,%d,%d\r\n", res.r, res.g, res.b);
	//UART_PUTF3("%d,%d,%d\r\n", res.r, res.g, res.b);

	return res;
}

// Count up the animation_position every 1/8000000 * 1024 * 256 ms = 32.768 ms,
// if animation is running.
ISR (TIMER2_OVF_vect)
{
	// TODO: Implement reverse direction.
	// TODO: Implement repeat.

	if (anim_len == 0) // no animation running
	{
		return;
	}
	
	if (anim_pos < anim_len)
	{
		anim_pos++;
	}
	else
	{
		UART_PUTF("Anim step %d finished.\r\n", anim_col_pos);
	
		if (anim_col_pos < 9)
		{
			anim_col_pos++;
			
			if ((9 == anim_col_pos) || (0 == anim_time[anim_col_pos])) // end of animation
			{
				UART_PUTF("set last: %d\r\n", anim_col_pos);
				
				if (anim_repeat == 1) // this was last run, end animation
				{
					set_PWM(anim_col[anim_col_pos]); // set color last time
					anim_len = 0;
					return;
				}
				else // more cycles to go (may also be an endless loop with anim_repeat == 0)
				{
					if (anim_repeat > 1)
						anim_repeat--;

					anim_col[1] = anim_col[anim_col_pos];
					anim_col_pos = 1;
					anim_pos = 0;
					anim_len = anim_cycles[anim_time[anim_col_pos]];
				}
			}
			else
			{
				anim_pos = 0;
				anim_len = anim_cycles[anim_time[anim_col_pos]];
			}
		}
		else
		{
			return; // don't change color at end of animation
		}
	}
	
	set_PWM(calc_pwm_color());
}

void set_animation_fixed_color(uint8_t color_index)
{
	cli();

	anim_time[0] = 0;
	anim_col_i[0] = color_index;
	anim_col[0] = index2color(color_index);
	anim_repeat = 1;
	anim_autoreverse = false;
	anim_len = 0;
	anim_pos = 0;
	anim_col_pos = 0;

	//UART_PUTF("Set color nr. %d\r\n", color_index);
	set_PWM(anim_col[0]);
	
	sei();
}

void dump_animation_values(void)
{
	uint8_t i;
	
	UART_PUTF2("Animation repeat: %d, autoreverse: %s, color/time: ",
		anim_repeat, anim_autoreverse ? "ON" : "OFF");
	
	for (i = 0; i < 10; i++)
	{
		UART_PUTF2("%d/%d", anim_col_i[i + 1], anim_time[i]);
		
		if (i < 9)
		{
			UART_PUTS(", ");
		}
	}

	UART_PUTS("\r\n");
}

void send_version_status(void)
{
	inc_packetcounter();

	UART_PUTF4("Sending Version: v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	// Set packet content
	pkg_header_init_generic_version_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_version_set_major(VERSION_MAJOR);
	msg_generic_version_set_minor(VERSION_MINOR);
	msg_generic_version_set_patch(VERSION_PATCH);
	msg_generic_version_set_hash(VERSION_HASH);
	pkg_header_calc_crc32();

	rfm12_send_bufx();
}

// Send color or coloranimation status, depending on whether an animation is running.
void send_status(void)
{
	inc_packetcounter();
	
	if (anim_len == 0) // no animation running
	{
		UART_PUTF("Sending color status: color: %u\r\n", anim_col_i[0]);
		
		// Set packet content
		pkg_header_init_dimmer_color_status();
		msg_dimmer_color_set_color(anim_col_i[0]);
	}
	else // animation running
	{
		uint8_t i;
		
		// Set packet content
		pkg_header_init_dimmer_coloranimation_status();
		
		UART_PUTF2("Sending animation status: Repeat: %u, AutoReverse: %u", anim_repeat, anim_autoreverse);
		msg_dimmer_coloranimation_set_repeat(anim_repeat);
		msg_dimmer_coloranimation_set_autoreverse(anim_autoreverse);
		
		for (i = 0; i < 10; i++)
		{
			UART_PUTF2(", Time[%u]: %u", i, anim_time[i]);
			UART_PUTF2(", Color[%u]: %u", i, anim_col_i[i]);
			msg_dimmer_coloranimation_set_color(i, anim_col_i[i]);
			msg_dimmer_coloranimation_set_time(i, anim_time[i]);
		}
		
		UART_PUTS("\r\n");
	}

	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	pkg_header_calc_crc32();
	rfm12_send_bufx();
}

// React accordingly on the MessageType, MessageGroup and MessageID.
void process_message(MessageTypeEnum messagetype, uint32_t messagegroupid, uint32_t messageid)
{
	UART_PUTF("MessageGroupID:%u;", messagegroupid);
	
	if (messagegroupid != MESSAGEGROUP_DIMMER)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		return;
	}
	
	UART_PUTF("MessageID:%u;", messageid);

	if ((messageid != MESSAGEID_DIMMER_COLOR) && (messageid != MESSAGEID_DIMMER_COLORANIMATION))
	{
		UART_PUTS("\r\nERR: Unsupported MessageID.\r\n");
		return;
	}

	// "Set" or "SetGet" -> modify color
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		if (messageid == MESSAGEID_DIMMER_COLORANIMATION)
		{
			uint8_t i;
			
			cli();
			
			anim_repeat = msg_dimmer_coloranimation_get_repeat();
			anim_autoreverse = msg_dimmer_coloranimation_get_autoreverse();
			anim_pos = 0;
			anim_col_pos = 0;
			anim_col[0] = index2color(0); // TODO: Set currently active color instead.
			
			UART_PUTF2("Repeat:%u;AutoReverse:%u;", anim_repeat, anim_autoreverse);
			
			for (i = 0; i < 10; i++)
			{
				anim_time[i] = msg_dimmer_coloranimation_get_time(i);
				anim_col_i[i] = msg_dimmer_coloranimation_get_color(i);
				anim_col[i + 1] = index2color(anim_col_i[i]);
				
				UART_PUTF2("Time[%u]:%u;", i, anim_time[i]);
				UART_PUTF2("Color[%u]:%u;", i, anim_col_i[i]);
			}
			
			anim_len = anim_cycles[anim_time[0]];
			set_PWM(calc_pwm_color());
			
			sei();
		}
		else
		{
			uint8_t color = msg_dimmer_color_get_color();
			UART_PUTF("Color:%u;", color);
			set_animation_fixed_color(color);
		}
	}

	UART_PUTS("\r\n");

	// remember some values before the packet buffer is destroyed
	uint32_t acksenderid = pkg_header_get_senderid();
	uint32_t ackpacketcounter = pkg_header_get_packetcounter();

	inc_packetcounter();

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
	{
		if (messageid == MESSAGEID_DIMMER_COLORANIMATION)
		{
			pkg_header_init_dimmer_coloranimation_ack();
		}
		else
		{
			pkg_header_init_dimmer_color_ack();
		}

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		if (messageid == MESSAGEID_DIMMER_COLORANIMATION)
		{
			uint8_t i;

			pkg_header_init_dimmer_coloranimation_ackstatus();
			
			for (i = 0; i < 10; i++)
			{
				msg_dimmer_coloranimation_set_color(i, anim_col_i[i]);
				msg_dimmer_coloranimation_set_time(i, anim_time[i]);
			}
			
			msg_dimmer_coloranimation_set_repeat(anim_repeat);
			msg_dimmer_coloranimation_set_autoreverse(anim_autoreverse);
		}
		else
		{
			pkg_header_init_dimmer_color_ackstatus();
			msg_dimmer_color_set_color(anim_col_i[0]);
		}
		
		// set message data
		UART_PUTS("Sending AckStatus\r\n");
	}

	// set common fields
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	
	pkg_headerext_common_set_acksenderid(acksenderid);
	pkg_headerext_common_set_ackpacketcounter(ackpacketcounter);
	pkg_headerext_common_set_error(false); // FIXME: Move code for the Ack to a function and also return an Ack when errors occur before!
	
	pkg_header_calc_crc32();
	
	rfm12_send_bufx();
	send_status_timeout = 5;
}

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
	uint8_t rcv_id = pkg_headerext_common_get_receiverid();

	UART_PUTF("ReceiverID:%u;", rcv_id);
	
	if (rcv_id != device_id)
	{
		UART_PUTS("\r\nWRN: DeviceID does not match.\r\n");
		return;
	}
	
	// check MessageGroup + MessageID
	uint32_t messagegroupid = pkg_headerext_common_get_messagegroupid();
	uint32_t messageid = pkg_headerext_common_get_messageid();
	
	process_message(messagetype, messagegroupid, messageid);
}

int main(void)
{
	uint8_t loop = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	
	check_eeprom_compatibility(DEVICETYPE_RGBDIMMER);
	
	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read last received station packetcounter
	station_packetcounter = e2p_rgbdimmer_get_basestationpacketcounter();
	
	// read device id
	device_id = e2p_generic_get_deviceid();

	brightness_factor = e2p_rgbdimmer_get_brightnessfactor();

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic RGB Dimmer v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2014 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n\r\n", station_packetcounter);
	UART_PUTF ("Brightness factor: %u%%\r\n", brightness_factor);

	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

	PWM_init();
	
	rfm12_init();
	
	set_animation_fixed_color(0);
	timer2_init();

	sei();

	while (42)
	{
		if (rfm12_rx_status() == STATUS_COMPLETE)
		{
			uint8_t len = rfm12_rx_len();
			
			if ((len == 0) || (len % 16 != 0))
			{
				UART_PUTF("Received garbage (%u bytes not multiple of 16): ", len);
				print_bytearray(bufx, len);
			}
			else // try to decrypt with all keys stored in EEPROM
			{
				UART_PUTS("\r\nReceived data!\r\n");
				
				memcpy(bufx, rfm12_rx_buffer(), len);
				memset(&bufx[len], 0, BUFX_LENGTH - len);
				
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

		// flash LED every second to show the device is alive
		if (loop == 50)
		{
			loop = 0;

			// send status from time to time
			send_status_timeout--;
		
			if (send_status_timeout == 0)
			{
				send_status_timeout = SEND_STATUS_EVERY_SEC;
				send_status();
				led_blink(200, 0, 1);
				
				version_status_cycle++;
			}
			else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
			{
				version_status_cycle = 0;
				send_version_status();
				led_blink(200, 0, 1);
			}
		}
		else
		{
			_delay_ms(20);
		}

		rfm12_tick();

		loop++;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
