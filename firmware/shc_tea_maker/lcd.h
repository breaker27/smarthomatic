/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2016 Uwe Freese
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

#include <avr/pgmspace.h>

extern char lcdbuf[];

#define LCD_PUTS(X)	                lcd_puts_p(PSTR((X)));
#define LCD_PUTF(X, A)              {sprintf_P(lcdbuf, PSTR((X)), (A)); lcd_putstr(lcdbuf);}
#define LCD_PUTF2(X, A, B)          {sprintf_P(lcdbuf, PSTR((X)), (A), (B)); lcd_putstr(lcdbuf);}
#define LCD_PUTF3(X, A, B, C)       {sprintf_P(lcdbuf, PSTR((X)), (A), (B), (C)); lcd_putstr(lcdbuf);}
#define LCD_PUTF4(X, A, B, C, D)    {sprintf_P(lcdbuf, PSTR((X)), (A), (B), (C), (D)); lcd_putstr(lcdbuf);}
#define LCD_PUTF5(X, A, B, C, D, E) {sprintf_P(lcdbuf, PSTR((X)), (A), (B), (C), (D), (E)); lcd_putstr(lcdbuf);}

void lcd_init(void);
void lcd_clear(void);
void lcd_putstr(char * str);
void lcd_puts_p(PGM_P str);
void lcd_gotoxy(unsigned char x, unsigned char y);
