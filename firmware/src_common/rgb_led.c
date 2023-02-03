/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2023 Uwe Freese
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

#include "rgb_led.h"
#include <avr/interrupt.h>
#include "uart.h"

uint8_t rgb_led_user_brightness_factor = 100; // additional brightness changeable by brightness message

struct rgb_color_t current_col; // The current (mixed) color calculated within an animation.
uint16_t current_tone_PWM;      // The current PWM value for the (slided) tone calculated within an melody.

// Timer0 (8 Bit) and Timer2 (8 Bit) are used for the PWM output for the LEDs
// and the interrupt that advances the values for the animation.
// Timer 1 (16 Bit) is used for the tone generator (speaker).
// Read for more information about PWM:
// http://www.protostack.com/blog/2011/06/atmega168a-pulse-width-modulation-pwm/
// http://extremeelectronics.co.in/avr-tutorials/pwm-signal-generation-by-using-avr-timers-part-ii/
void PWM_init(void)
{
	// clear animation and melody times so that no animation/melody begins to play
	uint8_t i;

	for (i = 0; i < ANIM_COL_MAX; i++)
		anim_time[i] = 0;

	for (i = 0; i < MELODY_TONE_MAX; i++)
		melody_time[i] = 0;

	// Enable output pins
	RGBLED_RED_DDR |= (1 << RGBLED_RED_PIN);
	RGBLED_GRN_DDR |= (1 << RGBLED_GRN_PIN);
	RGBLED_BLU_DDR |= (1 << RGBLED_BLU_PIN);
	SPEAKER_DDR    |= (1 << SPEAKER_PIN);

	// OC0A (Red LED):   Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	// OC0B (Green LED): Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	TCCR0A = (1 << WGM00) | (1 << COM0A1) | (1 << COM0B1);

	// OC2B (Blue LED):  Phase correct PWM, 8 Bit, TOP = 0xFF = 255, non-inverting output
	TCCR2A = (1 << WGM20) | (1 << COM2B1);

	// Clock source for timer 0 and 2 = I/O clock, 1/64 prescaler -> ~ 244 Hz
	TCCR0B = (1 << CS01) | (1 << CS00);
	TCCR2B = (1 << CS21) | (1 << CS20);

	// Timer/Counter1 Overflow Interrupt Enable
	// Timer0 is used for counting of the animation time as well
	TIMSK0 = (1 << TOIE0);

	// OC1A (Speaker):
	// Clock source for timer 1 = I/O clock, no prescaler
	// TOP = OCR1A register, toggle OC1A on compare match
	// 8 MHz: Fast PWM, 16 Bit. 20 MHz: Phase Correct PWM (2 times slower)
	TCCR1A = (1 << WGM11) | (1 << WGM10) | (1 << COM1A0);

#if (F_CPU == 8000000)
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
#elif (F_CPU == 20000000)
	TCCR1B = (1 << WGM13) (1 << CS10);
#endif

	// BLUE LED auf 2B, Speaker auf 1A
	// -> beides ändern bei RGB-Dimmer!
	// -> 2B ändern bei Controller!
}

void rgb_led_set_PWM(struct rgb_color_t color)
{
	OCR0A = (uint32_t)(pgm_read_word(&(rgb_led_pwm_transl[color.r]))) * rgb_led_brightness_factor * rgb_led_user_brightness_factor / 10000;
	OCR0B = (uint32_t)(pgm_read_word(&(rgb_led_pwm_transl[color.g]))) * rgb_led_brightness_factor * rgb_led_user_brightness_factor / 10000;
	OCR2B = (uint32_t)(pgm_read_word(&(rgb_led_pwm_transl[color.b]))) * rgb_led_brightness_factor * rgb_led_user_brightness_factor / 10000;
}

// Switch off speaker on special frequency_index == 0,
// or play the frequency with the given index.
void speaker_set_PWM(uint16_t PWM_val)
{
	if (PWM_val == 0)
	{
		SPEAKER_DDR &= ~ (1 << SPEAKER_PIN);
	}
	else
	{
		OCR1A = PWM_val;
		SPEAKER_DDR |= (1 << SPEAKER_PIN);
	}
}
// Switch off speaker on special frequency_index == 0,
// or play the frequency with the given index.
void speaker_set_fixed_tone(uint8_t frequency_index)
{
	if ((frequency_index == 0) || (frequency_index > 116))
	{
		speaker_set_PWM(0);
	}
	else
	{
		speaker_set_PWM(pgm_read_word(&(speaker_pwm_transl[frequency_index - 1])));
	}
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

uint16_t index2tonePWM(uint8_t frequency_index)
{
	if ((frequency_index == 0) || (frequency_index > 116))
	{
		return 0;
	}
	else
	{
		return pgm_read_word(&(speaker_pwm_transl[frequency_index - 1]));
	}
}

// Calculate the color that has to be shown according to the animation settings and counters.
// Save the color as currently active color in current_col.
void rgb_led_update_current_col(void)
{
	if (animation.step_len != 0) // animation running
	{
		current_col.r = (uint8_t)((uint32_t)anim_col[animation.col_pos].r * (animation.step_len - animation.step_pos) / animation.step_len
			+ (uint32_t)anim_col[animation.col_pos + 1].r * animation.step_pos / animation.step_len);
		current_col.g = (uint8_t)((uint32_t)anim_col[animation.col_pos].g * (animation.step_len - animation.step_pos) / animation.step_len
			+ (uint32_t)anim_col[animation.col_pos + 1].g * animation.step_pos / animation.step_len);
		current_col.b = (uint8_t)((uint32_t)anim_col[animation.col_pos].b * (animation.step_len - animation.step_pos) / animation.step_len
			+ (uint32_t)anim_col[animation.col_pos + 1].b * animation.step_pos / animation.step_len);
	}

	//UART_PUTF("animation.col_pos %d, ", animation.col_pos);
	//UART_PUTF3("PWM %d,%d,%d\r\n", current_col.r, current_col.g, current_col.b);

	rgb_led_set_PWM(current_col);
}

// Calculate the tone that has to be shown according to the animation settings and counters.
// Save the tone as currently active tone in current_tone.
void speaker_update_current_tone(void)
{
	if (melody.step_len != 0) // melody playing
	{
		current_tone_PWM = (uint8_t)((uint32_t)melody_PWM[melody.col_pos] * (melody.step_len - melody.step_pos) / melody.step_len
			+ (uint32_t)melody_PWM[melody.col_pos + 1] * melody.step_pos / melody.step_len);
	}

	//UART_PUTF("melody.col_pos %d, ", melody.col_pos);
	//UART_PUTF("PWM %d\r\n", current_tone_pwm);

	speaker_set_PWM(current_tone_PWM);
}

// Count up the animation_position every 1/8000000 * 1024 * 256 ms = 32.768 ms,
// if animation is running.
void animation_tick(bool RGB_LED)
{
	struct animation_param_t* par = RGB_LED ? &animation : &melody;
	uint8_t* array_time = RGB_LED ? anim_time : melody_time;

	if (par->step_len == 0) // no animation running / melody playing
	{
		return;
	}

	if (par->step_pos < par->step_len)
	{
		par->step_pos++;
	}
	else
	{
		UART_PUTF3("-- Anim step %d (color pos %d -> %d) finished.\r\n", par->col_pos, par->col_pos, par->col_pos + 1);

		// When animation step at animation.rlast is completed (animation.col_pos = animation.rlast) and
		// animation.repeat != 1, decrease animation.repeat by 1 (if not 0 already) and jump to animation.rfirst.
		if ((par->repeat != 1) && (par->col_pos == par->rlast))
		{
			par->col_pos++;

			if (par->repeat > 1)
					par->repeat--;

			UART_PUTF2("-- More cycles to go. New animation.repeat = %d. Reset animation.col_pos to %d.\r\n", par->repeat, par->rfirst);

			par->col_pos = par->rfirst;
			par->step_pos = 0;
			par->step_len = rgb_led_timer_cycles[array_time[par->col_pos]];
		}
		// When animation step at animation.llast is completed (animation.col_pos = animation.llast) and
		// animation.repeat = 1, stop animation.
		else if ((par->repeat == 1)  && (par->col_pos == par->llast))
		{
			UART_PUTF("-- End of animation. Set fixed color %d\r\n", par->col_pos + 1);

			// set color/tone last time
			if (RGB_LED)
				rgb_led_set_PWM(anim_col[par->col_pos + 1]);
			else
				speaker_set_PWM(melody_PWM[par->col_pos + 1]);

			par->step_len = 0;
			return;
		}
		else // Within animation -> go to next color
		{
			par->col_pos++;
			par->step_pos = 0;
			par->step_len = rgb_led_timer_cycles[array_time[par->col_pos]];

			UART_PUTF2("--- Go to next color, new animation.col_pos: %d. new animation.step_len: %d\r\n", par->col_pos, par->step_len);
		}
	}

	if (RGB_LED)
		rgb_led_update_current_col();
	else
		speaker_update_current_tone();
}

void rgb_led_set_fixed_color(uint8_t color_index)
{
	cli();

	animation.step_len = 0;
	current_col = index2color(color_index);
	anim_colors_orig[0] = color_index;
	anim_col[0] = current_col;

	//UART_PUTF("Set color nr. %d\r\n", color_index);
	rgb_led_update_current_col();

	sei();
}

/* unused DEBUG function
uint8_t find_last_animation.col_pos(void)
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
*/

void copy_reverse(bool RGB_LED, uint8_t from, uint8_t to, uint8_t count)
{
	//UART_PUTF3("Copy rev from %d to %d count %d\r\n", from, to, count);

	if (RGB_LED)
		for (int8_t i = 0; i < count; i++)
		{
			anim_col[to + count - 1 - i] = anim_col[from + i];
			anim_time[to + count - 1 - i] = anim_time[from + i - 1];
		}
	else
		for (int8_t i = 0; i < count; i++)
		{
			melody_PWM[to + count - 1 - i] = melody_PWM[from + i];
			melody_time[to + count - 1 - i] = melody_time[from + i - 1];
		}
}

void copy_forward(bool RGB_LED, uint8_t from, uint8_t to, uint8_t count)
{
	//UART_PUTF3("Copy fwd from %d to %d count %d\r\n", from, to, count);

	// copy in reverse direction because of overlapping range in scenario #4 "animation.autoreverse = 0, animation.repeat > 1"
	if (RGB_LED)
		for (int8_t i = count - 1; i >= 0; i--)
		{
			anim_col[to + i] = anim_col[from + i];
			anim_time[to + i] = anim_time[from + i];
		}
	else
		for (int8_t i = count - 1; i >= 0; i--)
		{
			melody_PWM[to + i] = melody_PWM[from + i];
			melody_time[to + i] = melody_time[from + i];
		}
}

// Set the RGB color values to play back according to the indexed colors.
// "Unfold" the colors to a linear animation only playing forward when
// animation.autoreverse is set.
void init_animation(bool RGB_LED)
{
	struct animation_param_t* par = RGB_LED ? &animation : &melody;
	uint8_t* array_time           = RGB_LED ? anim_time : melody_time;
	uint8_t pos_max               = RGB_LED ? ANIM_COL_ORIG_MAX : MELODY_TONE_ORIG_MAX;

	uint8_t key_idx = pos_max; // Marker for calculating how the values are copied (see doc).
	uint8_t i;

	par->step_pos = 0;
	par->col_pos = 0;

	// Transfer initial data to RGB array, shifted by 1.
	if (RGB_LED)
	{
		for (i = 0; i < pos_max; i++)
			anim_col[i + 1] = index2color(anim_colors_orig[i]);
		anim_col[0] = current_col;
	}
	else
	{
		for (i = 0; i < pos_max; i++)
			melody_PWM[i + 1] = index2tonePWM(melody_tones_orig[i]);
		melody_PWM[0] = current_tone_PWM;
	}

	// Find key_idx
	for (i = 0; i < pos_max; i++)
	{
		if (array_time[i] == 0)
		{
			key_idx = i;
			break;
		}
	}

	// calc animation.rfirst
	par->rfirst = par->autoreverse && (par->repeat % 2 == 1) ? key_idx - 1 : 2;

	// Copy data and set animation.rlast and animation.llast according "doc/initialization.png".
	if (par->autoreverse && (par->repeat != 1))
	{
		if (par->repeat == 0) // infinite cycles (picture #3)
		{
			copy_reverse(RGB_LED, 2, key_idx, key_idx - 1);
			par->rlast = par->llast = 2 * key_idx - 3;
		}
		else if (par->repeat % 2 == 0) // even number of cycles (picture #2)
		{
			uint8_t tmp_time = array_time[key_idx - 1];
			copy_forward(RGB_LED, key_idx, 2 * key_idx - 3, 1);
			copy_reverse(RGB_LED, 2, key_idx - 1, key_idx - 2);
			array_time[2 * key_idx - 4] = tmp_time;
			par->rlast = 2 * key_idx - 5;
			par->llast = 2 * key_idx - 4;
			par->repeat /= 2;
		}
		else // odd number of cycles (picture #1)
		{
			copy_forward(RGB_LED, 2, 2 * key_idx - 4, key_idx - 1);
			copy_reverse(RGB_LED, 3, key_idx - 1, key_idx - 3);
			par->rlast = 3 * key_idx - 8;
			par->llast = 3 * key_idx - 7;
			par->repeat /= 2;
		}
	}
	else
	{
		if (par->repeat == 0) // infinite cycles (picture #5)
		{
			copy_forward(RGB_LED, 2, key_idx + 1, 1);
			par->rlast = par->llast = key_idx;
			array_time[key_idx] = array_time[1];
		}
		else if (par->repeat == 1) // one cycle (picture #6)
		{
			par->rlast = par->llast = key_idx - 1;
		}
		else // 2 cycles or more (picture #4)
		{
			copy_forward(RGB_LED, 2, key_idx, key_idx - 1);
			array_time[key_idx - 1] = array_time[1];
			par->rlast = key_idx - 1;
			par->llast = 2 * key_idx - 3;
			par->repeat--;
		}
	}
	/*
	UART_PUTF("key_idx: %d\r\n", key_idx);
	UART_PUTF("animation.rfirst: %d\r\n", par->rfirst);
	UART_PUTF("animation.rlast: %d\r\n", par->rlast);
	UART_PUTF("animation.llast: %d\r\n", par->llast); */

	par->step_len = rgb_led_timer_cycles[array_time[0]];
}

// Print out the animation parameters, indexed colors used in the "Set"/"SetGet" message and the
// calculated RGB colors.
// Only for debugging purposes!
void dump_animation_values(void)
{
	uint8_t i;

	UART_PUTF2("Animation repeat: %d, autoreverse: %s, ",
		animation.repeat, animation.autoreverse ? "ON" : "OFF");
	UART_PUTF3("Current color: RGB %3d,%3d,%3d\r\n", current_col.r, current_col.g, current_col.b);

	UART_PUTF3("animation.rfirst: %d, animation.rlast: %d, animation.llast: %d\r\n",
		animation.rfirst, animation.rlast, animation.llast);

	for (i = 0; i < ANIM_COL_MAX; i++)
	{
		UART_PUTF("Pos %02d: color ", i);

		if (i < 10)
		{
			UART_PUTF("%02d   ", anim_colors_orig[i]);
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
	animation.repeat = 1;
	animation.autoreverse = false;

	animation.step_pos = 0;
	animation.col_pos = 0;

	anim_time[0] = 10;
	anim_colors_orig[0] = 0;
	anim_time[1] = 16;
	anim_colors_orig[1] = 48;
	anim_time[2] = 16;
	anim_colors_orig[2] = 12;
	anim_time[3] = 16;
	anim_colors_orig[3] = 3;
	anim_time[4] = 15;
	anim_colors_orig[4] = 51;
	anim_time[5] = 9;
	anim_colors_orig[5] = 1;

	UART_PUTS("\r\n*** Initial colors ***\r\n");
	dump_animation_values();

	init_animation(true);

	UART_PUTS("\r\n*** Colors after initialisation ***\r\n");
	dump_animation_values();

	animation.step_len = rgb_led_timer_cycles[anim_time[0]];

	while (42) {}
}
