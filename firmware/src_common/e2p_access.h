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

#ifndef E2P_ACCESS_H
#define E2P_ACCESS_H

#ifndef UNITTEST
#include <avr/eeprom.h>
#endif

#include <stdint.h>

#ifdef UNITTEST

uint8_t eeprom_read_byte (const uint8_t *_p);
void eeprom_write_byte (const uint8_t *_p, uint8_t byte);
void signal_error_state(void);

#endif

#ifndef NULL
	#define NULL ((void*)0) 
#endif

uint32_t _eeprom_read_UIntValue32(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval, uint16_t max_bits_for_type, uint8_t * array);
int32_t _eeprom_read_IntValue32(uint16_t bit, uint16_t length, int32_t minval, int32_t maxval, uint8_t * array);

void _eeprom_write_UIntValue(uint16_t bit, uint16_t length, uint32_t val, uint8_t * array);

// function wrappers for EEPROM access
static inline uint8_t eeprom_read_UIntValue8(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval)
{
	return (uint8_t)_eeprom_read_UIntValue32(bit, length, minval, maxval, 8, NULL);
}

static inline uint16_t eeprom_read_UIntValue16(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval)
{
	return (uint16_t)_eeprom_read_UIntValue32(bit, length, minval, maxval, 16, NULL);
}

static inline uint32_t eeprom_read_UIntValue32(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval)
{
	return _eeprom_read_UIntValue32(bit, length, minval, maxval, 32, NULL);
}

static inline int32_t eeprom_read_IntValue32(uint16_t bit, uint16_t length, int32_t minval, int32_t maxval)
{
	return _eeprom_read_IntValue32(bit, length, minval, maxval, NULL);
}

static inline void eeprom_write_UIntValue(uint16_t bit, uint16_t length, uint32_t val)
{
	_eeprom_write_UIntValue(bit, length, val, NULL);
}

static inline void eeprom_write_IntValue(uint16_t bit, uint16_t length, int32_t val)
{
	// move the sign bit of the standard int type to the sign bit position of our variable-sized int type
	_eeprom_write_UIntValue(bit, length,
		(((val >> 31) & 1) << (length - 1)) | (val & (((uint32_t)1 << (length - 1)) - 1)),
		NULL);
}

// function wrappers for ARRAY access
static inline uint8_t array_read_UIntValue8(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval, uint8_t * array)
{
	return (uint8_t)_eeprom_read_UIntValue32(bit, length, minval, maxval, 8, array);
}

static inline uint16_t array_read_UIntValue16(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval, uint8_t * array)
{
	return (uint16_t)_eeprom_read_UIntValue32(bit, length, minval, maxval, 16, array);
}

static inline uint32_t array_read_UIntValue32(uint16_t bit, uint16_t length, uint32_t minval, uint32_t maxval, uint8_t * array)
{
	return _eeprom_read_UIntValue32(bit, length, minval, maxval, 32, array);
}

static inline int32_t array_read_IntValue32(uint16_t bit, uint16_t length, int32_t minval, int32_t maxval, uint8_t * array)
{
	return _eeprom_read_IntValue32(bit, length, minval, maxval, array);
}

static inline void array_write_UIntValue(uint16_t bit, uint16_t length, uint32_t val, uint8_t * array)
{
	_eeprom_write_UIntValue(bit, length, val, array);
}

static inline void array_write_IntValue(uint16_t bit, uint16_t length, int32_t val, uint8_t * array)
{
	// move the sign bit of the standard int type to the sign bit position of our variable-sized int type
	_eeprom_write_UIntValue(bit, length,
		(((val >> 31) & 1) << (length - 1)) | (val & (((uint32_t)1 << (length - 1)) - 1)),
		array);
}

#endif // E2P_ACCESS
