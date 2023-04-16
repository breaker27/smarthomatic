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

#include "vlcd.h"
#include "lcd.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

char* vlcdbuf = "*l*c*d**b*u*f*f*e*r**l*c*d**b*u*f*f*e*r*";

#define VIRTUAL_LCD_PAGES         3
#define VIRTUAL_LCD_SCREEN_HEIGHT 4 * VIRTUAL_LCD_PAGES
#define VIRTUAL_LCD_SCREEN_WIDTH  40
char lcd_data[VIRTUAL_LCD_SCREEN_HEIGHT][VIRTUAL_LCD_SCREEN_WIDTH];

uint8_t vlcd_chars_per_line = 20;
uint8_t current_page = 0;

uint8_t vlcd_lastx, vlcd_y, vlcd_x;

void clear_lcd_data(void)
{
	for (uint8_t y = 0; y < VIRTUAL_LCD_SCREEN_HEIGHT; y++)
		for (uint8_t x = 0; x < VIRTUAL_LCD_SCREEN_WIDTH; x++)
			lcd_data[y][x] = '\x20';
}

void vlcd_init(bool big4x40)
{
	clear_lcd_data();

	if (big4x40)
		vlcd_chars_per_line = 40;
	else
		vlcd_chars_per_line = 20;

	lcd_init(big4x40);
}

void vlcd_clear(void)
{
	clear_lcd_data();
	lcd_clear();
}

void vlcd_clear_page(uint8_t page)
{
	for (uint8_t y = 0; y < 4; y++)
		for (uint8_t x = 0; x < VIRTUAL_LCD_SCREEN_WIDTH; x++)
			lcd_data[page * 4 + y][x] = '\x20';

	if (page == current_page)
		lcd_clear();

	lcd_clear();
}

void vlcd_putc(char c)
{
	if (vlcd_x < vlcd_chars_per_line)
	{
		lcd_data[vlcd_y][vlcd_x] = c;

		if (vlcd_y / 4 == current_page)
			lcd_putc(c);

		vlcd_x++;
	}
}

void vlcd_puts(const char* s)
{
	while (*s)
	{
		if (*s == '\n')
		{
			if (vlcd_y < VIRTUAL_LCD_SCREEN_HEIGHT)
				vlcd_gotoyx(vlcd_y + 1, vlcd_lastx);
		}
		else if (vlcd_x < vlcd_chars_per_line)
		{
			lcd_data[vlcd_y][vlcd_x] = *s;

			if (vlcd_y / 4 == current_page)
				lcd_putc(*s);

			vlcd_x++;
		}

		s++;
	}
}

void vlcd_gotoyx(uint8_t y, uint8_t x)
{
	vlcd_y = y;
	vlcd_lastx = vlcd_x = x;

	if (y / 4 == current_page)
		lcd_gotoyx(y % 4, x);
}

void vlcd_set_page(uint8_t page)
{
	if ((page < VIRTUAL_LCD_PAGES) && (current_page != page))
	{
		current_page = page;

		for (uint8_t y = 0; y < 4; y++)
		{
			lcd_gotoyx(y, 0);

			for (uint8_t x = 0; x < VIRTUAL_LCD_SCREEN_WIDTH; x++)
				lcd_putc(lcd_data[page * 4 + y][x]);
		}
	}
}

uint8_t vlcd_get_page(void)
{
	return current_page;
}
