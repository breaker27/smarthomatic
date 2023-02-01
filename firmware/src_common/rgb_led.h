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

#ifndef _RGB_LED_H
#define _RGB_LED_H

#include <avr/io.h>
#include <inttypes.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

// For the given 5 bit animation time, these are the lengths in timer 2 cycles.
// The input value x means 0.05s * 1.3 ^ x and covers 30ms to 170s. Each timer cycle is 32.768ms.
// Therefore, the values are: round((0.05s * 1.3 ^ x) / 0.032768s).
// Animation time 0 is animation OFF, so there are 31 defined animation times.
static const uint16_t rgb_led_timer_cycles[31] = {
	1, 2, 3, 4, 6, 8, 10, 12, 16, 21, 27, 36, 46, 60,
	78, 102, 132, 172, 223, 290, 377, 490, 637, 828,
	1077, 1400, 1820, 2366, 3075, 3998, 5197};

// https://www.nongnu.org/avr-libc/user-manual/pgmspace.html
static const uint16_t rgb_led_pwm_transl[256] PROGMEM = {
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

// PWM TOP values to generate frequencies according calculations in Extra/frequencies.ods
#if (F_CPU == 8000000)
static const uint16_t speaker_pwm_transl[116] PROGMEM = {
	61152, 57719, 54480, 51426, 48537, 45813, 43242, 40815, 38524, 36363, 34322, 32396,
	30578, 28861, 27241, 25713, 24269, 22907, 21621, 20407, 19262, 18181, 17160, 16197,
	15288, 14430, 13620, 12855, 12134, 11453, 10810, 10203, 9631, 9090, 8580, 8098,
	7644, 7214, 6809, 6427, 6066, 5726, 5404, 5101, 4815, 4544, 4289, 4049, 3821,
	3607, 3404, 3213, 3033, 2862, 2702, 2550, 2407, 2272, 2144, 2024, 1910, 1803,
	1702, 1606, 1516, 1431, 1350, 1275, 1203, 1135, 1072, 1011, 955, 901, 850,
	803, 757, 715, 675, 637, 601, 567, 535, 505, 443, 399, 363, 332, 307, 285,
	266, 249, 234, 221, 210, 199, 189, 181, 173, 166, 159, 153, 147, 142, 137,
	132, 128, 124, 120, 117, 113, 110, 107, 104, 102, 99 };
#elif (F_CPU == 20000000)
// the first three tones are one octave higher, because the value for the low frequency is larger than 16 bits
static const uint16_t speaker_pwm_transl[116] PROGMEM = {
	38222, 36077, 34052, 64283, 60671, 57266, 54053, 51019, 48155, 45454, 42903, 40495,
	38222, 36077, 34052, 32141, 30337, 28634, 27026, 25509, 24078, 22726, 21451, 20247,
	19110, 18038, 17025, 16069, 15168, 14316, 13513, 12754, 12038, 11363, 10725, 10123,
	9555, 9018, 8512, 8034, 7583, 7158, 6756, 6377, 6019, 5681, 5362, 5061, 4777,
	4509, 4256, 4017, 3791, 3578, 3377, 3188, 3009, 2840, 2680, 2530, 2388, 2254,
	2127, 2008, 1895, 1789, 1688, 1593, 1504, 1419, 1340, 1264, 1193, 1126, 1063,
	1003, 947, 894, 844, 796, 751, 709, 669, 632, 555, 499, 454, 416, 384,
	356, 332, 312, 293, 277, 262, 249, 237, 226, 216, 207, 199, 191, 184,
	178, 171, 166, 160, 155, 151, 146, 142, 138, 134, 131, 127, 124 };
#else
	#error Only 8000000 or 20000000 are allowed as F_CPU
#endif

struct rgb_color_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

// parameters for the color animation and the tone "animation" (melody)
struct animation_param_t
{
	uint8_t  repeat;      // Number of repeats, 0 = endless.
	bool     autoreverse; // Play back animation in reverse order when finished.

	uint16_t step_len;    // length of animation step of current color to next color.
	uint16_t step_pos;    // Position (= number of times the animation tick was progressed) in the current animation step.
	uint8_t  col_pos;     // Index of currently animated color.

	uint8_t  rfirst;      // When a repeat loop active, where does a cycle start (depends on animation.autoreverse switch).
	uint8_t  rlast;       // When a repeat loop active, where does a cycle end (depends on repeat count).
	uint8_t  llast;       // In the last cycle, where does it end (depends on repeat count).
};

struct animation_param_t animation;
struct animation_param_t melody;

uint8_t rgb_led_brightness_factor;        // fixed, from e2p
uint8_t rgb_led_user_brightness_factor;   // additional brightness changeable by brightness message

#define ANIM_COL_MAX 31             // 3x animation length + 1 to unfold the color sequence
struct rgb_color_t anim_col[ANIM_COL_MAX]; // The last active color (index 0) + colors used for the animation. Unfolded.
uint8_t anim_time[ANIM_COL_MAX];    // The times used for animate blending between two colors. Unfolded.
                                    // 0 at pos x indicates the last color used is x-1.
uint8_t anim_colors_orig[10];       // The 10 (indexed) colors used for the animation.

#define MELODY_TONE_MAX 88          // 3x melody length + 1 to unfold the tone sequence
uint8_t melody_tone[MELODY_TONE_MAX];  // The last active tone (index 0) + tones used for the melody. Unfolded.
uint8_t melody_slide[MELODY_TONE_MAX]; // The slide mode last active tone (index 0) + values used for the melody. Unfolded.
uint8_t melody_time[MELODY_TONE_MAX];  // The times used for playing a tone / sliding between two tones. Unfolded.
                                    // 0 at pos x indicates the last tone used is x-1.
uint8_t melody_tones_orig[10];      // The 29 (indexed) tones used for the melody, as stated in the SHC message.

// function definitions
void PWM_init(void);

void rgb_led_set_PWM(struct rgb_color_t color);
struct rgb_color_t index2color(uint8_t color);
void rgb_led_update_current_col(void);
void rgb_led_animation_tick(void);
void rgb_led_set_fixed_color(uint8_t color_index);
void init_animation(void);
void test_anim_calculation(void);

void speaker_set_fixed_tone(uint8_t frequency_index);

#endif /* _RGB_LED_H */
