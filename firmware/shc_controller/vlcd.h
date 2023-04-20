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

// Virtual LCD
// This has more than the physical LCD lines and can display one of the virtual screens.

#ifndef _VLCD_H
#define _VLCD_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

#define VIRTUAL_LCD_PAGES 5

extern char* vlcdbuf;
extern uint8_t vlcd_chars_per_line;

#define VLCD_PUTS(X)	             {strcpy_P(vlcdbuf, PSTR((X))); vlcd_puts(vlcdbuf);}
#define VLCD_PUTF(X, A)              {sprintf_P(vlcdbuf, PSTR((X)), (A)); vlcd_puts(vlcdbuf);}
#define VLCD_PUTF2(X, A, B)          {sprintf_P(vlcdbuf, PSTR((X)), (A), (B)); vlcd_puts(vlcdbuf);}
#define VLCD_PUTF3(X, A, B, C)       {sprintf_P(vlcdbuf, PSTR((X)), (A), (B), (C)); vlcd_puts(vlcdbuf);}
#define VLCD_PUTF4(X, A, B, C, D)    {sprintf_P(vlcdbuf, PSTR((X)), (A), (B), (C), (D)); vlcd_puts(vlcdbuf);}
#define VLCD_PUTF5(X, A, B, C, D, E) {sprintf_P(vlcdbuf, PSTR((X)), (A), (B), (C), (D), (E)); vlcd_puts(vlcdbuf);}

void vlcd_init(bool big4x40);
void vlcd_clear(void);
void vlcd_clear_page(uint8_t page);
void vlcd_putc(char c);
void vlcd_puts(const char* s);
void vlcd_gotoyx(uint8_t y, uint8_t x);
void vlcd_set_page(uint8_t page);
uint8_t vlcd_get_page(void);

#endif /* _VLCD_H */
