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

#include "lcd.h"
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>

// This buffer is used for sending strings to the LCD.
char lcdbuf[128];

/* Anschluss des Displays über 4bit-Interface
DB4-DB7       PC0-PC3
RS            PC4
E             PC5
*/

/* User Characters are stored in EEPROM at pos. 0-6

// Ohm Character

0b00000000 = 0x00
0b00001110 = 0x0E
0b00010001 = 0x11
0b00010001 = 0x11
0b00010001 = 0x11
0b00001010 = 0x0A
0b00011011 = 0x1B
0b00000000 = 0x00

*/

#define LCD_SET_FUNCTION        0x20
 
#define LCD_FUNCTION_4BIT       0x00
#define LCD_FUNCTION_2LINE      0x08
#define LCD_FUNCTION_5X7        0x00

#define LCD_SET_ENTRY           0x04
#define LCD_ENTRY_INCREASE      0x02
#define LCD_ENTRY_NOSHIFT       0x00

#define LCD_SET_DISPLAY         0x08
 
#define LCD_DISPLAY_OFF         0x00
#define LCD_DISPLAY_ON          0x04
#define LCD_CURSOR_OFF          0x00
#define LCD_CURSOR_ON           0x02
#define LCD_BLINKING_OFF        0x00
#define LCD_BLINKING_ON         0x01

#define LCD_DDRAM 7
#define LCD_START_LINE1 0x00
#define LCD_START_LINE2 0x40

#define LCD_PORT PORTA
#define LCD_DDR DDRA
#define LCD_PIN_EN	1
#define LCD_PIN_RS  0

void lcd_enable(void)
{
    LCD_PORT |= (1 << LCD_PIN_EN);
    _delay_us(20);
    LCD_PORT &= ~(1 << LCD_PIN_EN);
}

void lcd_out(uint8_t data)
{
    data &= 0xF0; // select upper 4 bits
    LCD_PORT &= ~(0xF0 >> 2); // set 4 bits for D4..D7
    LCD_PORT |= (data >> 2);
    lcd_enable();
}

void lcd_data(unsigned char c)
{
	LCD_PORT |= (1 << LCD_PIN_RS); // RS == 1 (write data byte)
 
    lcd_out(c);
    lcd_out(c << 4);
 
    _delay_us(50);
}

void lcd_command(unsigned char c)
{
	LCD_PORT &= ~(1 << LCD_PIN_RS); // RS == 0 (write command byte)
 
    lcd_out(c);
    lcd_out(c << 4);
 
    _delay_us(50);
}

void lcd_init(void)
{
	unsigned char c;
	unsigned int i;
	
	// set some I/O pins for LCD to output mode
	LCD_DDR = 0b00111111;

	_delay_ms(20); // startup delay
	
	// muss 3mal hintereinander gesendet werden zur Initialisierung
	lcd_out(0x30);
	_delay_ms(5);
	lcd_enable();
	_delay_ms(5);
	lcd_enable();
	_delay_ms(5);

	// set 4 bit mode
	lcd_out(LCD_SET_FUNCTION | LCD_FUNCTION_4BIT);
	_delay_ms(5);

    // set display format
    lcd_command(LCD_SET_FUNCTION | LCD_FUNCTION_4BIT | LCD_FUNCTION_2LINE | LCD_FUNCTION_5X7);
 
    // configure cursor
    lcd_command(LCD_SET_DISPLAY | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINKING_OFF); 
 
    // configure cursor movement
    lcd_command(LCD_SET_ENTRY | LCD_ENTRY_INCREASE | LCD_ENTRY_NOSHIFT );
 
	lcd_clear();

	// read user characters from eeprom and write to lcd
	/*lcd_command((1<<3) | 0b01000000);
	sleep(5);
	
	for (i = 0; i < 8 * 7; i++)
	{
		c = eeprom_read_uchar(i);

	//sprintf(lcdbuf, "CHAR %u is %u\n", i, c);
	//uart_puts(lcdbuf);

		lcd_data(c);
		sleep(5);
	}
	*/
}

void lcd_clear(void)
{
	lcd_command(0b00000001);
    _delay_ms(5);
}

void lcd_putc(char c)
{
	switch ((uint8_t)c)
	{
		case 0xb0: // °
			lcd_data(0xdf);
			break;
		case 0xe4: // ä
			lcd_data(0xe1);
			break;
		case 0xf6: // ö
			lcd_data(0xef);
			break;
		case 0xfc: // ü
			lcd_data(0xf5);
			break;
		case 0xc4: // Ä
			lcd_data(0x00); // user character 0
			break;
		case 0xd6: // Ö
			lcd_data(0x01); // user character 1
			break;
		case 0xdc: // Ü
			lcd_data(0x02); // user character 2
			break;
		case 0xdf: // ß
			lcd_data(0xe2);
			break;
		default:
			lcd_data(c);
			break;
	}
}

void lcd_putstr(char * str)
{
	while(*str)
	{
		lcd_putc(*str++);
	}
}

void lcd_puts_p(PGM_P str)
{
	char tmp;

	while ((tmp = pgm_read_byte(str)))
	{
		lcd_putc(tmp);
		str++;
	}
}

void lcd_gotoxy(unsigned char x, unsigned char y)
{
	if (y == 0)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE1 + x);
	else
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE2 + x);
}
