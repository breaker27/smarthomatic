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

#ifndef _LCD_H
#define _LCD_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

void lcd_init(bool big4x40);
void lcd_clear(void);
void lcd_putc(char c);
void lcd_puts(const char* s);
void lcd_gotoyx(uint8_t y, uint8_t x);

#endif /* _LCD_H */
