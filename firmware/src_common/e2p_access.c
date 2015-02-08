/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
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

#include "e2p_access.h"
#include "util_generic.h"

// #define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#ifdef UNITTEST

#include <stdio.h> // for using printf-debugging

uint8_t e2p[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255, 254, 253, 252, 251, 250, 249, 248 };

uint8_t eeprom_read_byte(const uint8_t *_p)
{
	uint32_t p = (uint32_t)_p;
	return e2p[p];
}

void eeprom_write_byte(const uint8_t *_p, uint8_t byte)
{
	uint32_t p = (uint32_t)_p;
	e2p[p] = byte;
}

void signal_error_state(void)
{
	//exit(1);
}

#endif

/* ---------- internal functions ---------- */

// Limit the value the pointer directs to between minval and maxval.
void __limitUIntValue32(uint32_t *val, uint32_t minval, uint32_t maxval)
{
	if (*val < minval)
	{
		*val = minval;
	}
	
	if (*val > maxval)
	{
		*val = maxval;
	}
}

// Limit the value the pointer directs to between minval and maxval.
void __limitIntValue32(int32_t *val, int32_t minval, int32_t maxval)
{
	if (*val < minval)
	{
		*val = minval;
	}
	
	if (*val > maxval)
	{
		*val = maxval;
	}
}

// clear some bits within a byte
uint8_t __clear_bits(uint8_t input, uint8_t bit, uint8_t bits_to_clear)
{
	uint8_t mask = ~((((1 << bits_to_clear) - 1)) << (8 - bits_to_clear - bit));
	return input & mask;
}

// get some bits from a 32 bit value, counted from the left (MSB) side! The first bit is bit nr. 0.
uint8_t __get_bits(uint32_t input, uint8_t bit, uint8_t length)
{
	return (input >> (32 - length - bit)) & ((1 << length) - 1);
}

// Read UIntValue from EEPROM.
// Go into error mode if bit length is not as assumed.
uint32_t __eeprom_read_UIntValue32(uint16_t bit, uint16_t length, uint16_t max_bits_for_type, uint8_t * array)
{
	uint16_t byte = bit / 8;
	bit = bit % 8;
	
	if (length > max_bits_for_type)
	{
		signal_error_state();
	}

	uint8_t byres_read = 0;
	uint32_t val = 0;
	int8_t shift;
	
	// read the bytes one after another, shift them to the correct position and add them
	while (length + bit > byres_read * 8)
	{
		shift = length + bit - byres_read * 8 - 8;
		uint32_t zz = (NULL == array) ? eeprom_read_byte((uint8_t*)(byte + byres_read)) : array[byte + byres_read];

		if (shift >= 0)
		{
			val += zz << shift;
		}
		else
		{
			val += zz >> - shift;
		}

		byres_read++;
	}

	// filter out only the wanted bits and clear unwanted upper bits
	if (length < 32)
	{
		val = val & (((uint32_t)1 << length) - 1);
	}

	return val;
}

// write some bits to EEPROM only within one byte
void __eeprom_write_bits(uint16_t bit, uint16_t length, uint8_t val, uint8_t * array)
{
	uint16_t byte = bit / 8;
	bit = bit % 8;

//	printf("Write value %d to byte %d bit %d with length %d\n", val, byte, bit, length);
	
	uint8_t b = 0;
	
	// if length is smaller than 8 bits, get the old value from eeprom
	if (length < 8)
	{
		b = (NULL == array) ? eeprom_read_byte((uint8_t*)(byte + 0)) : array[byte];	
		b = __clear_bits(b, bit, length);
	}
	
	// set bits from given value
	b = b | (val << (8 - length - bit));

	if (NULL == array)
	{
		eeprom_write_byte((uint8_t*)(byte + 0), b);
	}
	else
	{
		array[byte] = b;
	}
}

/* ---------- exported functions ---------- */

// Read UIntValue from EEPROM and limit it into the given boundaries.
uint32_t _eeprom_read_UIntValue32(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval, uint16_t max_bits_for_type, uint8_t * array)
{
	uint32_t x = __eeprom_read_UIntValue32(bit, length, max_bits_for_type, array);
	
	__limitUIntValue32(&x, minval, maxval);
	return x;
}

// Read IntValue from EEPROM and limit it into the given boundaries.
int32_t _eeprom_read_IntValue32(uint16_t bit, uint16_t length, int32_t minval, int32_t maxval, uint8_t * array)
{
	uint32_t x = __eeprom_read_UIntValue32(bit, length, 32, array);

	// If MSB is 1 (value is negative interpreted as signed int),
	// set all higher bits also to 1.
	if (((x >> (length - 1)) & 1) == 1)
	{
		x = x | ~(((uint32_t)1 << (length - 1)) - 1);
	}

	int32_t y = (int32_t)x;
	
	__limitIntValue32(&y, minval, maxval);
	return y;
}

// Write UIntValue to EEPROM.
void _eeprom_write_UIntValue(uint16_t bit, uint16_t length, uint32_t val, uint8_t * array)
{
	uint16_t byte = bit / 8;
	bit = bit % 8;

	// move bits to the left border
	val = val << (32 - length);

	//UART_PUTF("Moved left: val %lu\r\n", val);	
	
	// 1st byte
	uint8_t src_start = 0;
	uint8_t dst_start = bit;
	uint8_t len = MIN(length, 8 - bit);
	uint8_t val8 = __get_bits(val, src_start, len);
	
	//UART_PUTF4("Write bits to byte %u, dst_start %u, len %u, val8 %u\r\n", byte, dst_start, len, val8);	
	
	__eeprom_write_bits(byte * 8 + dst_start, len, val8, array);
	
	dst_start = 0;
	src_start = len;

	while (src_start < length)
	{
		len = MIN(length - src_start, 8);
		val8 = __get_bits(val, src_start, len);
		byte++;

		//UART_PUTF4(" Byte nr. %d, src_start %d, len %d, val8 %d\r\n", byte, src_start, len, val8);

		__eeprom_write_bits(byte * 8 + dst_start, len, val8, array);
		
		src_start += len;
	}
}
