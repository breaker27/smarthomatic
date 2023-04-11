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

#include "lcd.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

#include "../src_common/uart.h"

bool double_controller = false;
uint8_t last_controller_nr = 0;
uint8_t lcd_chars_per_line = 20;
uint8_t last_y = 0;
uint8_t last_x = 0;

// Definition of characters (max 8) which are not in the font
uint8_t user_chars[] = {
	// Ä
	0b00010001,
	0b00001110,
	0b00010001,
	0b00010001,
	0b00011111,
	0b00010001,
	0b00010001,
	0b00000000,
	// Ö
	0b00010001,
	0b00001110,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00001110,
	0b00000000,
	// Ü
	0b00010001,
	0b00000000,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00001110,
	0b00000000,
	// Battery Empty Character
	0b00001110,
	0b00011111,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00011111,
	0b00000000,
	// Battery 25% Full Character
	0b00001110,
	0b00011111,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00011111,
	0b00011111,
	0b00000000,
	// Battery 50% Full Character
	0b00001110,
	0b00011111,
	0b00010001,
	0b00010001,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00000000,
	// Battery 75% Full Character
	0b00001110,
	0b00011111,
	0b00010001,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00000000,
	// Battery Full Character
	0b00001110,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00000000
};

#define LCD_DDRAM           7
#define LCD_START_LINE1     0x00
#define LCD_START_LINE2     0x40

#define LCD_DATA_PORT       PORTC // IO pin 4-7 are used for LCD D4-D7
#define LCD_DATA_DDR        DDRC
#define LCD_RS_PORT         PORTA
#define LCD_EN_PORT         PORTA
#define LCD_RS_DDR          DDRA
#define LCD_EN_DDR          DDRA
#define LCD_RS_PIN          5
#define LCD_EN1_PIN         6
#define LCD_EN2_PIN         7

#define LCD_CMD_CLEAR 0b00000001

#define sbi(ADDRESS,BIT) ((ADDRESS) |= (1<<(BIT)))
#define cbi(ADDRESS,BIT) ((ADDRESS) &= ~(1<<(BIT)))

void lcd_rs(bool send_text)
{
	if (send_text)
		sbi(LCD_RS_PORT, LCD_RS_PIN);
	else // send command
		cbi(LCD_RS_PORT, LCD_RS_PIN);
}

void lcd_en(void)
{
	if (last_controller_nr == 0)
	{
		sbi(LCD_EN_PORT, LCD_EN1_PIN);
		_delay_us(5);
		cbi(LCD_EN_PORT, LCD_EN1_PIN);
		_delay_us(5);
	}
	else
	{
		sbi(LCD_EN_PORT, LCD_EN2_PIN);
		_delay_us(5);
		cbi(LCD_EN_PORT, LCD_EN2_PIN);
		_delay_us(5);
	}
}

// if send_data = false, send a command
void lcd_send(bool send_data, unsigned char c)
{
	lcd_rs(send_data);
	LCD_DATA_PORT = (c & 0b11110000) | (LCD_DATA_PORT & 0b00001111); // send 4 upper bits
	lcd_en();
	LCD_DATA_PORT = ((c << 4) & 0b11110000) | (LCD_DATA_PORT & 0b00001111); // send 4 lower bits
	lcd_en();
	_delay_us(50); // 50µs
}

inline void lcd_command(unsigned char c)
{
	lcd_send(false, c);
}

inline void lcd_data(unsigned char c)
{
	lcd_send(true, c);
}

void lcd_init_internal(uint8_t controller_nr)
{
	unsigned int i;

	last_controller_nr = controller_nr;

	// muss 3mal hintereinander gesendet werden zur Initialisierung ?????????????????? OBSOLET???
	LCD_DATA_PORT = 0b00110000 | (LCD_DATA_PORT & 0b00001111);
	lcd_en();
	_delay_ms(5);
	lcd_en();
	_delay_ms(5);
	lcd_en();
	_delay_ms(5);

	// Initialize 4 Bit mode
	LCD_DATA_PORT = 0b00100000 | (LCD_DATA_PORT & 0b00001111);
	lcd_en();
	_delay_ms(5);

	// 4 Bit data length, 2-line display, 5x7 Font
	lcd_command(0b00101000);

	// Display on, Cursor off, Cursor blink off
	lcd_command(0b00001100);

	// Entry mode set: auto shift cursor
	lcd_command(0b00000100);

	lcd_clear();

	// write user characters to LCD
	lcd_command(0b01000000);
	_delay_ms(1);

	for (i = 0; i < sizeof(user_chars); i++)
	{
		lcd_data(user_chars[i]);
	}
	_delay_ms(1);

	lcd_clear();
}

void lcd_init(bool big4x40)
{
	double_controller = big4x40;

	// only two types supported currently (4x20, 4x40)
	if (double_controller)
		lcd_chars_per_line = 40;
	else
		lcd_chars_per_line = 20;

	sbi(LCD_DATA_DDR, 4);
	sbi(LCD_DATA_DDR, 5);
	sbi(LCD_DATA_DDR, 6);
	sbi(LCD_DATA_DDR, 7);
	sbi(LCD_RS_DDR, LCD_RS_PIN);
	sbi(LCD_EN_DDR, LCD_EN1_PIN);

	if (big4x40)
		sbi(LCD_EN_DDR, LCD_EN2_PIN);

	_delay_ms(550);

	lcd_init_internal(0);

	if (big4x40)
		lcd_init_internal(1);
}


void lcd_clear()
{
	last_controller_nr = 0;
	lcd_command(LCD_CMD_CLEAR);
    _delay_ms(5);

	if (double_controller)
	{
		last_controller_nr = 1;
		lcd_command(LCD_CMD_CLEAR);
		_delay_ms(5);
	}
}

void lcd_putc(char c)
{
	UART_PUTF("CHAR %u\r\n", (uint8_t)c);

	switch ((uint8_t)c)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			c -= 1;
			break;
		case 228: // ä
			c = 225;
			break;
		case 246: // ö
			c = 239;
			break;
		case 252: // ü
			c = 245;
			break;
		case 223: // ß
			c = 226;
			break;
		case 196: // Ä
			c = 0;
			break;
		case 214: // Ö
			c = 1;
			break;
		case 220: // Ü
			c = 2;
			break;
	}

	lcd_data(c);
}

// Print string on LCD. Support newline to go to next line at old x position.
void lcd_puts(const char* s)
{
	while (*s)
		lcd_putc(*s++);
}

// 1st character for x and y has index 0
void lcd_gotoyx(uint8_t y, uint8_t x)
{
	if (double_controller)
	{
		last_controller_nr = y / 2;
	}
	else
	{
		if (y >= 2)
			x += lcd_chars_per_line;
	}

	if (y % 2 == 0)
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE1 + x);
	else
		lcd_command((1 << LCD_DDRAM) + LCD_START_LINE2 + x);

	last_y = y;
	last_x = x;
}
