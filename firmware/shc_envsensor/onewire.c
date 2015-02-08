/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Uwe Freese
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

#include "onewire.h"

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#include "../src_common/uart.h"

// The onewire port pin is always set to 0.
// To drive physical pin low as an output, set DDR to 1 (= output).
// To let physical pin be an open drain, set DDR to 0 (= input / floating).

#define ONEWIRE_LOW    ONEWIRE_DDR |= (1 << ONEWIRE_PIN);
#define ONEWIRE_OPEN   ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);

//#define SREG_I 7 // global interrupt enable bit in SREG

// detect sensor type by family code in ROM ID
#define ONEWIRE_FAMILY_CODE_DS18S20    0x10
#define ONEWIRE_FAMILY_CODE_DS18B20    0x28

// one-wire commands
#define CMD_READ_ROM                   0x33
#define CMD_MATCH_ROM                  0x55
#define CMD_CONVERT_T                  0x44
#define CMD_READ_SCRATCHPAD            0xbe

bool onewire_error = false;
bool onewire_global_interrupts_enabled;

/* ********************* Low level read/write functions. *********************
*
* Calling a 1-wire command always works as follows using these functions:
*
* 1) Disable interrupts
* 2) Call onewire_reset()
* 3) write and read some bytes as needed by the slave device
* 4) Enable interrupts
*/

// Return 1 if no slave found.
bool onewire_reset(void)
{
	uint8_t tmp;

	ONEWIRE_LOW
	_delay_us(500); // min. 480us
	ONEWIRE_OPEN
	_delay_us(65);  // min. 60us

	tmp = ONEWIRE_PINREG & (1 << ONEWIRE_PIN); // check for slave pulling low
	_delay_us(430); // min. 480us incl. the ~60us above

	return tmp;
}

void onewire_write_bit(uint8_t bit)
{
	ONEWIRE_LOW

	if (bit)
	{
		ONEWIRE_OPEN
	}

	_delay_us(105);
	ONEWIRE_OPEN
}

void onewire_write_byte(uint8_t byte)
{
	uint8_t i;

	// write 8 bits, beginning with the LSB
	for (i = 0; i < 8; i++)
	{
		onewire_write_bit((byte >> i) & 1);
	}
}

uint8_t onewire_read_bit(void)
{
	uint8_t tmp;

	ONEWIRE_LOW
	ONEWIRE_OPEN    
	_delay_us(15);

	tmp = ONEWIRE_PINREG & (1 << ONEWIRE_PIN);
	_delay_us(105);

	return tmp;
}

uint8_t onewire_read_byte(void)
{
	uint8_t i;
	uint8_t res = 0;

	// read 8 bits, starting with the LSB
	for (i = 0; i < 8; i++)
	{
		if (onewire_read_bit())
		{
			res |= 1 << i;
		}
	}

	return res;
}

void disable_global_interrupts(void)
{
	onewire_global_interrupts_enabled = (SREG >> SREG_I) & 1;
	cli();
}

void enable_global_interrupts(void)
{
	if (onewire_global_interrupts_enabled)
	{
		sei();
	}
}

// ********************* High level user functions. *********************

void onewire_init(void)
{
	// pin value is always zero to drive physical pin low as soon as DDR is set as output
	ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
}

bool onewire_get_rom_id(uint8_t * id_array)
{
	uint8_t i;

	disable_global_interrupts();
	
	if (onewire_reset())
	{
		enable_global_interrupts();
		return true;
	}

	onewire_write_byte(CMD_READ_ROM);

	// store answer (ROM ID)
	for (i = 0; i < 8; i++)
	{
		id_array[i] = onewire_read_byte();
	}
	
	enable_global_interrupts();
	return false;
}

bool _onewire_send_cmd(uint8_t * id_array, uint8_t cmd)
{
	uint8_t i;
	
	if (onewire_reset())
	{
		return true;
	}

	onewire_write_byte(CMD_MATCH_ROM);

	// send ROM ID
	for (i = 0; i < 8; i++)
	{
		onewire_write_byte(id_array[i]);
	}
	
	onewire_write_byte(cmd);
	
	return false;
}

int16_t onewire_get_temperature(uint8_t * id_array)
{
	uint8_t i;
	int16_t res;
	uint8_t tmp[8];

	disable_global_interrupts();
	
	if (_onewire_send_cmd(id_array, CMD_CONVERT_T))
	{
		enable_global_interrupts();
		return NO_TEMPERATURE;
	}

	enable_global_interrupts();

	_delay_ms(800);

	disable_global_interrupts();

	if (_onewire_send_cmd(id_array, CMD_READ_SCRATCHPAD))
	{
		enable_global_interrupts();
		return NO_TEMPERATURE;
	}

	// store scratchpad (temperature value)
	for (i = 0; i < 8; i++)
	{
		tmp[i] = onewire_read_byte();
	}

	enable_global_interrupts();
	
	// Calculate temperature according datasheet.
	// Detect sensor type by family code.
	switch (id_array[0])
	{
		case ONEWIRE_FAMILY_CODE_DS18S20:
			// get temperature raw value considering sign bits
			if (tmp[1] == 0)
				res = tmp[0];
			else
				res = -256 + tmp[0];
			
			// calculate temperate using COUNT_PER_C and COUNT_REMAIN for higher resolution
			res = (int16_t)((int32_t)res * 100 / 2 - 25 + ((int32_t)tmp[7] - tmp[6]) * 100 / tmp[7]);
			break;
			
		case ONEWIRE_FAMILY_CODE_DS18B20:
			// get temperature raw value
			res = ((uint16_t)(tmp[1] & 0b00000111) << 8) + tmp[0];

			// clear undefined bits if lower resolution is configured in DS18B20 e2p
			uint8_t undefined_bits = 3 - ((tmp[4] >> 5) & 3);
			res = res & (0b1111111111111111 << undefined_bits);
			
			if (tmp[1] >> 7 == 1)
				res = -2048 + res;

			res = (int16_t)((int32_t)res * 625 / 100);
			break;
			
		default: res = NO_TEMPERATURE;
	}

	return res;
}
