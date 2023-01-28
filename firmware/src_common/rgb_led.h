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

// For the given 5 bit animation time, these are the lengths in timer 2 cycles.
// The input value x means 0.05s * 1.3 ^ x and covers 30ms to 170s. Each timer cycle is 32.768ms.
// Therefore, the values are: round((0.05s * 1.3 ^ x) / 0.032768s).
// Animation time 0 is animation OFF, so there are 31 defined animation times.
static const uint16_t rgb_led_timer_cycles[31] = {1, 2, 3, 4, 6, 8, 10, 12, 16, 21, 27, 36, 46, 60,
                                  78, 102, 132, 172, 223, 290, 377, 490, 637, 828,
                                  1077, 1400, 1820, 2366, 3075, 3998, 5197};

static const uint16_t rgb_led_pwm_transl[256] = {
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

uint8_t rgb_led_brightness_factor;        // fixed, from e2p
uint8_t rgb_led_user_brightness_factor;   // additional brightness changeable by brightness message

#define ANIM_COL_MAX 31
struct rgb_color_t anim_col[ANIM_COL_MAX]; // The last active color (index 0) + colors used for the animation
uint8_t anim_time[ANIM_COL_MAX];    // The times used for animate blending between two colors.
                                    // 0 at pos x indicates the last color used is x-1.
uint8_t rgb_led_anim_colors[10];    // The 10 (indexed) colors used for the animation.
uint8_t rgb_led_repeat;    // Number of repeats, 0 = endless.
bool rgb_led_autoreverse;  // Play back animation in reverse order when finished.
uint16_t step_len;         // length of animation step of current color to next color.

// function definitions
void PWM_init(void);
void timer2_init(void);
void set_PWM(struct rgb_color_t color);
struct rgb_color_t index2color(uint8_t color);
void rgb_led_update_current_col(void);
void rgb_led_animation_tick(void);
void rgb_led_set_fixed_color(uint8_t color_index);
void init_animation(void);
void clear_anim_data(void);
void test_anim_calculation(void);

#endif /* _RGB_LED_H */
