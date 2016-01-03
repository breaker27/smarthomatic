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
#include "../src_common/uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_dimmer.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_rgbdimmer.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

#include "../src_common/util_watchdog_init.h"

#define RGBLED_DDR DDRD
#define RGBLED_PORT PORTD
#define RGBLED_PINPORT PIND

// RFM12 NRES (Reset) pin may be connected to PC3.
// If not, only sw reset is used.
#define RFM_RESET_PIN 3
#define RFM_RESET_PORT_NR 1

#define SEND_STATUS_EVERY_SEC 2400 // how often should a status be sent?
#define SEND_VERSION_STATUS_CYCLE 35 // send version status x times less than switch status (~once per day)

uint16_t device_id;
uint32_t station_packetcounter;

uint16_t send_status_timeout = 15;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

uint8_t brightness_factor;            // fixed, from e2p
uint8_t user_brightness_factor = 100; // additional brightness changeable by brightness message

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
const uint16_t timer_cycles[31] = {1, 2, 3, 4, 6, 8, 10, 12, 16, 21, 27, 36, 46, 60,
                                  78, 102, 132, 172, 223, 290, 377, 490, 637, 828,
                                  1077, 1400, 1820, 2366, 3075, 3998, 5197};

const uint16_t pwm_transl[256] = {
	0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7,
	8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14,
	14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20,
	21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 27, 27, 27,
	28, 28, 28, 29, 29, 30, 30, 30, 31, 31, 32, 32, 32, 33, 33, 34, 34, 34, 35, 35,
	36, 36, 37, 37, 38, 38, 38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45,
	46, 46, 47, 47, 48, 48, 49, 49, 50, 51, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
	58, 58, 59, 60, 60, 61, 62, 63, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73,
	73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 87, 88, 89, 90, 91, 92, 94,
	95, 96, 97, 99, 100, 101, 103, 104, 105, 107, 108, 110, 111, 113, 114, 116, 118,
	119, 121, 123, 124, 126, 128, 130, 132, 134, 135, 137, 139, 141, 144, 146, 148,
	150, 152, 155, 157, 159, 162, 164, 167, 169, 172, 174, 177, 180, 183, 185, 188,
	191, 194, 197, 200, 204, 207, 210, 214, 217, 221, 224, 228, 231, 235, 239, 243,
	247, 251, 255};

struct rgb_color_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

#define ANIM_COL_MAX 31
struct rgb_color_t anim_col[ANIM_COL_MAX]; // The last active color (index 0) + colors used for the animation
struct rgb_color_t current_col;            // The current (mixed) color calculated within an animation.
uint8_t anim_time[ANIM_COL_MAX];           // The times used for animate blending between two colors.
                                           // 0 at pos x indicates the last color used is x-1.
uint8_t anim_col_i[10];    // The 10 (indexed) colors used for the animation.
uint8_t repeat;            // Number of repeats, 0 = endless.
bool autoreverse;          // Play back animation in reverse order when finished.
uint16_t step_len = 0;     // length of animation step of current color to next color.
uint16_t step_pos = 0;     // Position in the current animation step.
uint8_t col_pos = 0;       // Index of currently animated color.
uint8_t rfirst;            // When a repeat loop active, where does a cycle start (depends on autoreverse switch).
uint8_t rlast;             // When a repeat loop active, where does a cycle end (depends on repeat count).
uint8_t llast;             // In the last cycle, where does it end (depends on repeat count).

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

	// OC0A (Red LED): Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	// OC0B (Green LED): Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	TCCR0A = (1 << WGM00) | (1 << COM0A1) | (1 << COM0B1);

	// OC1A (Blue LED): Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	TCCR1A = (1 << WGM10) | (1 << COM1A1);

	// Clock source for both timers = I/O clock, 1/64 prescaler -> ~ 240 Hz
	TCCR0B = (1 << CS01) | (1 << CS00);
	TCCR1B = (1 << CS11) | (1 << CS10);
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
	OCR0A = (uint32_t)(pwm_transl[color.r]) * brightness_factor * user_brightness_factor / 10000;
	OCR0B = (uint32_t)(pwm_transl[color.g]) * brightness_factor * user_brightness_factor / 10000;
	OCR1A = (uint32_t)(pwm_transl[color.b]) * brightness_factor * user_brightness_factor / 10000;
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
	
	//UART_PUTF4("Index %d to color -> %d,%d,%d\r\n", color, res.r, res.g, res.b);
	
	return res;
}

// Calculate the color that has to be shown according to the animation settings and counters.
// Save the color as currently active color in current_col.
void update_current_col(void)
{
	if (step_len != 0) // animation running
	{
		current_col.r = (uint8_t)((uint32_t)anim_col[col_pos].r * (step_len - step_pos) / step_len
			+ (uint32_t)anim_col[col_pos + 1].r * step_pos / step_len);
		current_col.g = (uint8_t)((uint32_t)anim_col[col_pos].g * (step_len - step_pos) / step_len
			+ (uint32_t)anim_col[col_pos + 1].g * step_pos / step_len);
		current_col.b = (uint8_t)((uint32_t)anim_col[col_pos].b * (step_len - step_pos) / step_len
			+ (uint32_t)anim_col[col_pos + 1].b * step_pos / step_len);
	}

	//UART_PUTF("col_pos %d, ", col_pos);
	//UART_PUTF3("PWM %d,%d,%d\r\n", current_col.r, current_col.g, current_col.b);
	
	set_PWM(current_col);
}

// Count up the animation_position every 1/8000000 * 1024 * 256 ms = 32.768 ms,
// if animation is running.
ISR (TIMER2_OVF_vect)
{
	if (step_len == 0) // no animation running
	{
		return;
	}
	
	if (step_pos < step_len)
	{
		step_pos++;
	}
	else
	{
		//UART_PUTF3("-- Anim step %d (color pos %d -> %d) finished.\r\n", col_pos, col_pos, col_pos + 1);

		// When animation step at rlast is completed (col_pos = rlast) and
		// repeat != 1, decrease repeat by 1 (if not 0 already) and jump to rfirst.
		if ((repeat != 1) && (col_pos == rlast))
		{
			col_pos++;

			if (repeat > 1)
					repeat--;

			//UART_PUTF2("-- More cycles to go. New repeat = %d. Reset col_pos to %d.\r\n", repeat, rfirst);
			
			col_pos = rfirst;
			step_pos = 0;
			step_len = timer_cycles[anim_time[col_pos]];
		}
		// When animation step at llast is completed (col_pos = llast) and
		// repeat = 1, stop animation.
		else if ((repeat == 1)  && (col_pos == llast))
		{
			//UART_PUTF("-- End of animation. Set fixed color %d\r\n", col_pos + 1);
			set_PWM(anim_col[col_pos + 1]); // set color last time
			step_len = 0;
			return;
		}
		else // Within animation -> go to next color
		{
			col_pos++;
			step_pos = 0;
			step_len = timer_cycles[anim_time[col_pos]];
			
			//UART_PUTF2("--- Go to next color, new col_pos: %d. new step_len: %d\r\n", col_pos, step_len);
		}
	}
	
	update_current_col();
}

void set_animation_fixed_color(uint8_t color_index)
{
	cli();

	step_len = 0;
	current_col = index2color(color_index);
	anim_col_i[0] = color_index;
	anim_col[0] = current_col;

	//UART_PUTF("Set color nr. %d\r\n", color_index);
	update_current_col();
	
	sei();
}

void send_deviceinfo_status(void)
{
	inc_packetcounter();

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_RGBDIMMER);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_RGBDIMMER);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	rfm12_send_bufx();
}

// Send brightness status
void send_brightness_status(void)
{
	inc_packetcounter();

	UART_PUTF("Sending color status: color: %u\r\n", anim_col_i[0]);

	// Set packet content
	pkg_header_init_dimmer_brightness_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_dimmer_brightness_set_brightness(user_brightness_factor);

	rfm12_send_bufx();
}

// Send color or coloranimation status, depending on whether an animation is running.
void send_status(void)
{
	inc_packetcounter();
	
	if (step_len == 0) // no animation running
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
		
		UART_PUTF2("Sending animation status: Repeat: %u, AutoReverse: %u", repeat, autoreverse);
		msg_dimmer_coloranimation_set_repeat(repeat);
		msg_dimmer_coloranimation_set_autoreverse(autoreverse);
		
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
	rfm12_send_bufx();
}

uint8_t find_last_col_pos(void)
{
	uint8_t i;
	
	for (i = 0; i < ANIM_COL_MAX; i++)
	{
		if (anim_time[i] == 0)
		{
			return i - 1;
		}
	}
	
	return ANIM_COL_MAX - 1;
}

void copy_reverse(uint8_t from, uint8_t to, uint8_t count)
{
	int8_t i;
	
	//UART_PUTF3("Copy rev from %d to %d count %d\r\n", from, to, count);
	
	for (i = 0; i < count; i++)
	{
		anim_col[to + count - 1 - i] = anim_col[from + i];
		anim_time[to + count - 1 - i] = anim_time[from + i - 1];
	}
}

void copy_forward(uint8_t from, uint8_t to, uint8_t count)
{
	int8_t i;
	
	//UART_PUTF3("Copy fwd from %d to %d count %d\r\n", from, to, count);
	
	// copy in reverse direction because of overlapping range in scenario #4 "autoreverse = 0, repeat > 1"
	for (i = count - 1; i >= 0; i--)
	{
		anim_col[to + i] = anim_col[from + i];
		anim_time[to + i] = anim_time[from + i];
	}
}

// Set the RGB color values to play back according to the indexed colors.
// "Unfold" the colors to a linear animation only playing forward when
// autoreverse is set.
void init_animation(void)
{
	uint8_t key_idx = 10; // Marker for calculating how the values are copied (see doc).
	uint8_t i;
	
	// Transfer initial data to RGB array, shifted by 1.
	for (i = 0; i < 10; i++)
	{
		anim_col[i + 1] = index2color(anim_col_i[i]);
	}

	anim_col[0] = current_col;
	
	// Find key_idx
	for (i = 0; i < 10; i++)
	{
		if (anim_time[i] == 0)
		{
			key_idx = i;
			break;
		}
	}
	
	// calc rfirst
	rfirst = autoreverse && (repeat % 2 == 1) ? key_idx - 1 : 2;
	
	// Copy data and set rlast and llast according "doc/initialization.png".
	if (autoreverse && (repeat != 1))
	{
		if (repeat == 0) // infinite cycles (picture #3)
		{
			copy_reverse(2, key_idx, key_idx - 1);
			rlast = llast = 2 * key_idx - 3;
		}
		else if (repeat % 2 == 0) // even number of cycles (picture #2)
		{
			uint8_t tmp_time = anim_time[key_idx - 1];
			copy_forward(key_idx, 2 * key_idx - 3, 1);
			copy_reverse(2, key_idx - 1, key_idx - 2);
			anim_time[2 * key_idx - 4] = tmp_time;
			rlast = 2 * key_idx - 5;
			llast = 2 * key_idx - 4;
			repeat /= 2;
		}
		else // odd number of cycles (picture #1)
		{
			copy_forward(2, 2 * key_idx - 4, key_idx - 1);
			copy_reverse(3, key_idx - 1, key_idx - 3);
			rlast = 3 * key_idx - 8;
			llast = 3 * key_idx - 7;
			repeat /= 2;
		}
	}
	else
	{
		if (repeat == 0) // infinite cycles (picture #5)
		{
			copy_forward(2, key_idx + 1, 1);
			rlast = llast = key_idx;
			anim_time[key_idx] = anim_time[1];
		}
		else if (repeat == 1) // one cycle (picture #6)
		{
			rlast = llast = key_idx - 1;
		}
		else // 2 cycles or more (picture #4)
		{
			copy_forward(2, key_idx, key_idx - 1);
			anim_time[key_idx - 1] = anim_time[1];
			rlast = key_idx - 1;
			llast = 2 * key_idx - 3;
			repeat--;
		}
	}
	/*
	UART_PUTF("key_idx: %d\r\n", key_idx);
	UART_PUTF("rfirst: %d\r\n", rfirst);
	UART_PUTF("rlast: %d\r\n", rlast);
	UART_PUTF("llast: %d\r\n", llast); */
}

void clear_anim_data(void)
{
	uint8_t i;
	
	for (i = 0; i < ANIM_COL_MAX; i++)
	{
		anim_time[i] = 0;
	}
}

// Print out the animation parameters, indexed colors used in the "Set"/"SetGet" message and the
// calculated RGB colors.
// Only for debugging purposes!
void dump_animation_values(void)
{
	uint8_t i;
	
	UART_PUTF2("Animation repeat: %d, autoreverse: %s, ",
		repeat, autoreverse ? "ON" : "OFF");
	UART_PUTF3("Current color: RGB %3d,%3d,%3d\r\n", current_col.r, current_col.g, current_col.b);

	UART_PUTF3("rfirst: %d, rlast: %d, llast: %d\r\n",
		rfirst, rlast, llast);

	for (i = 0; i < ANIM_COL_MAX; i++)
	{
		UART_PUTF("Pos %02d: color ", i);
	
		if (i < 10)
		{
			UART_PUTF("%02d   ", anim_col_i[i]);
		}
		else
		{
			UART_PUTS("--   ");
		}

		UART_PUTF3("RGB %3d,%3d,%3d", anim_col[i].r, anim_col[i].g, anim_col[i].b);
		
		UART_PUTF("   time %02d\r\n", anim_time[i]);
	}

	UART_PUTS("\r\n");
}

// Function to test the calculation of animation colors.
// Call it at the beginning of the main function for debugging.
void test_anim_calculation(void)
{
	// change repeat and autoreverse to check the different scenarios
	repeat = 0;
	autoreverse = true;
	
	step_pos = 0;
	col_pos = 0;
			
	anim_time[0] = 10;
	anim_col_i[0] = 0;
	anim_time[1] = 16;
	anim_col_i[1] = 48;
	anim_time[2] = 16;
	anim_col_i[2] = 12;
	anim_time[3] = 16;
	anim_col_i[3] = 3;
	anim_time[4] = 15;
	anim_col_i[4] = 51;
	anim_time[5] = 9;
	anim_col_i[5] = 1;

	UART_PUTS("\r\n*** Initial colors ***\r\n");
	dump_animation_values();
	
	init_animation();
	
	UART_PUTS("\r\n*** Colors after initialisation ***\r\n");
	dump_animation_values();
	
	step_len = timer_cycles[anim_time[0]];

	while (42) {}
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
	
	if (messagegroupid != MESSAGEGROUP_DIMMER)
	{
		UART_PUTS("\r\nERR: Unsupported MessageGroupID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}
	
	UART_PUTF("MessageID:%u;", messageid);

	if ((messageid != MESSAGEID_DIMMER_BRIGHTNESS)
		&& (messageid != MESSAGEID_DIMMER_COLOR)
		&& (messageid != MESSAGEID_DIMMER_COLORANIMATION))
	{
		UART_PUTS("\r\nERR: Unsupported MessageID.\r\n");
		send_ack(acksenderid, ackpacketcounter, true);
		return;
	}

	// "Set" or "SetGet" -> modify brightness/color/animation
	if ((messagetype == MESSAGETYPE_SET) || (messagetype == MESSAGETYPE_SETGET))
	{
		if (messageid == MESSAGEID_DIMMER_BRIGHTNESS)
		{
			user_brightness_factor = msg_dimmer_brightness_get_brightness();
			UART_PUTF("Brightness:%u;", brightness_factor);
			update_current_col();
		}
		else if (messageid == MESSAGEID_DIMMER_COLOR)
		{
			uint8_t color = msg_dimmer_color_get_color();
			UART_PUTF("Color:%u;", color);
			set_animation_fixed_color(color);
		}
		else // MESSAGEID_DIMMER_COLORANIMATION
		{
			uint8_t i;
			
			cli();
			
			repeat = msg_dimmer_coloranimation_get_repeat();
			autoreverse = msg_dimmer_coloranimation_get_autoreverse();
			step_pos = 0;
			col_pos = 0;
			anim_col[0] = current_col;
			
			UART_PUTF2("Repeat:%u;AutoReverse:%u;", repeat, autoreverse);
			
			for (i = 0; i < 10; i++)
			{
				anim_time[i] = msg_dimmer_coloranimation_get_time(i);
				anim_col_i[i] = msg_dimmer_coloranimation_get_color(i);
				
				UART_PUTF2("Time[%u]:%u;", i, anim_time[i]);
				UART_PUTF2("Color[%u]:%u;", i, anim_col_i[i]);
			}
			
			init_animation();
			step_len = timer_cycles[anim_time[0]];
			update_current_col();
			
			sei();
		}
	}

	UART_PUTS("\r\n");

	// "Set" -> send "Ack"
	if (messagetype == MESSAGETYPE_SET)
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

		UART_PUTS("Sending Ack\r\n");
	}
	// "Get" or "SetGet" -> send "AckStatus"
	else
	{
		if (messageid == MESSAGEID_DIMMER_BRIGHTNESS)
		{
			pkg_header_init_dimmer_brightness_ackstatus();
			msg_dimmer_brightness_set_brightness(user_brightness_factor);
		}
		else if (messageid == MESSAGEID_DIMMER_COLOR)
		{
			pkg_header_init_dimmer_color_ackstatus();
			msg_dimmer_color_set_color(anim_col_i[0]);
		}
		else // MESSAGEID_DIMMER_COLORANIMATION
		{
			uint8_t i;

			pkg_header_init_dimmer_coloranimation_ackstatus();
			
			for (i = 0; i < 10; i++)
			{
				msg_dimmer_coloranimation_set_color(i, anim_col_i[i]);
				msg_dimmer_coloranimation_set_time(i, anim_time[i]);
			}
			
			msg_dimmer_coloranimation_set_repeat(repeat);
			msg_dimmer_coloranimation_set_autoreverse(autoreverse);
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
	UART_PUTS("(c) 2014..2015 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("Last received base station PacketCounter: %u\r\n\r\n", station_packetcounter);
	UART_PUTF ("E2P brightness factor: %u%%\r\n", brightness_factor);

	// init AES key
	e2p_generic_get_aeskey(aes_key);

	PWM_init();
	
	rfm_watchdog_init(device_id, e2p_rgbdimmer_get_transceiverwatchdogtimeout(), RFM_RESET_PORT_NR, RFM_RESET_PIN);
	rfm12_init();
	
	set_animation_fixed_color(0);
	timer2_init();

	clear_anim_data();
	// test_anim_calculation(); // for debugging only

	// Show colors shortly to tell user that power is connected (status LED may not be visible).
	// In parallel, let status LED blink 3 times (as usual for SHC devices).
	current_col = index2color(48);
	set_PWM(current_col);
	switch_led(true);
	_delay_ms(500);
	current_col = index2color(12);
	set_PWM(current_col);
	switch_led(false);
	_delay_ms(500);
	current_col = index2color(3);
	set_PWM(current_col);
	switch_led(true);
	_delay_ms(500);
	current_col = index2color(0);
	set_PWM(current_col);
	switch_led(false);
	_delay_ms(500);

	led_blink(500, 0, 1);

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
				UART_PUTS("\r\nReceived data!\r\n");
				
				memcpy(bufx, rfm12_rx_buffer(), len);
				memset(&bufx[len], 0, BUFX_LENGTH - len);
				
				//UART_PUTS("Before decryption: ");
				//print_bytearray(bufx, len);
					
				aes256_decrypt_cbc(bufx, len);

				//UART_PUTS("Decrypted bytes: ");
				//print_bytearray(bufx, len);
				
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
			if (!send_startup_reason(&mcusr_mirror))
			{
				send_status_timeout--;
			
				if (send_status_timeout == 8)
				{
					send_brightness_status();
					led_blink(200, 0, 1);
				}
				else if (send_status_timeout == 0)
				{
					send_status_timeout = SEND_STATUS_EVERY_SEC;
					send_status();
					led_blink(200, 0, 1);
					
					version_status_cycle++;
				}
				else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
				{
					version_status_cycle = 0;
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

		loop++;
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
