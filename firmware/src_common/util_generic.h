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

// Utility functions are split up into util_*.{c|h} files.
// The util.{c|h} files in the devices directories include all of them.
// Only one lib is therefore built that contains all functions.

#ifndef _UTIL_GENERIC_H
#define _UTIL_GENERIC_H

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

// used as buffer for sending data to SHT
#define BUFX_LENGTH 65
uint8_t bufx[BUFX_LENGTH];

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// ########## Linear Interpolation

uint16_t linear_interpolate16(uint16_t in, uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out);
uint32_t linear_interpolate32(uint32_t in, uint32_t min_in, uint32_t max_in, uint32_t min_out, uint32_t max_out);
float linear_interpolate_f(float in, float min_in, float max_in, float min_out, float max_out);

// ########## Hex -> Dec conversion

uint8_t hex_to_byte(char c);
uint8_t hex_to_uint8(uint8_t * buf, uint8_t offset);

static inline uint16_t hex_to_uint16(uint8_t * buf, uint8_t offset)
{
	return ((uint16_t)hex_to_uint8(buf, offset) << 8)
		+ hex_to_uint8(buf, offset + 2);
}

static inline uint32_t hex_to_uint24(uint8_t * buf, uint8_t offset)
{
	return ((uint32_t)hex_to_uint8(buf, offset) << 16)
		+ ((uint32_t)hex_to_uint8(buf, offset + 2) << 8)
		+ hex_to_uint8(buf, offset + 4);
}

static inline uint32_t hex_to_uint32(uint8_t * buf, uint8_t offset)
{
	return ((uint32_t)hex_to_uint8(buf, offset) << 24)
		+ ((uint32_t)hex_to_uint8(buf, offset + 2) << 16)
		+ ((uint32_t)hex_to_uint8(buf, offset + 4) << 8)
		+ hex_to_uint8(buf, offset + 6);
}

// ########## read/write 16 bit / 32 bit values to the byte buffer in PC byte order

uint32_t getBuf16(uint8_t offset);
uint32_t getBuf32(uint8_t offset);
void setBuf32(uint8_t offset, uint32_t val);
void setBuf16(uint8_t offset, uint16_t val);

// ########## CRC32

uint32_t crc32(uint8_t *data, uint8_t len);

#endif /* _UTIL_GENERIC_H */
