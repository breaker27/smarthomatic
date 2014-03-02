/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
*
* Contributors:
*    CRC32: K.Moraw, www.helitron.de, 2009
*
* Development for smarthomatic by Uwe Freese started by adding a
* function to encrypt and decrypt a byte buffer directly and adding CBC mode.
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


#include "util_generic.h"

// PRECONDITION (which is NOT checked!): min_in < max_in, min_out < max_out
uint16_t linear_interpolate16(uint16_t in, uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

uint32_t linear_interpolate32(uint32_t in, uint32_t min_in, uint32_t max_in, uint32_t min_out, uint32_t max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

float linear_interpolate_f(float in, float min_in, float max_in, float min_out, float max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

// Take one characters and return the hex value it represents (0..15).
// If characters are not 0..9, a..f, A..F, character is interpreted as 0xf.
// 0 = 48, 9 = 57
// A = 65, F = 70
// a = 97, f = 102
uint8_t hex_to_byte(char c)
{
	if (c <= 48) // 0
	{
		return 0;
	}
	else if (c <= 57) // 1..9
	{
		return c - 48;
	}
	else if (c <= 65) // A
	{
		return 10;
	}
	else if (c <= 70) // B..F
	{
		return c - 55;
	}
	else if (c <= 97) // a
	{
		return 10;
	}
	else if (c <= 102) // b..f
	{
		return c - 87;
	}
	else // f
	{
		return 15;
	}		
}

uint8_t hex_to_uint8(uint8_t * buf, uint8_t offset)
{
	return hex_to_byte(buf[offset]) * 16 + hex_to_byte(buf[offset + 1]);
}

// Return a 32 bit value stored in 4 bytes in the buffer at the given offset.
uint32_t getBuf32(uint8_t offset)
{
	return ((uint32_t)bufx[offset] << 24) | ((uint32_t)bufx[offset + 1] << 16) | ((uint32_t)bufx[offset + 2] << 8) | ((uint32_t)bufx[offset + 3] << 0);
}

// Return a 16 bit value stored in 2 bytes in the buffer at the given offset.
uint32_t getBuf16(uint8_t offset)
{
	return ((uint32_t)bufx[offset] << 8) | ((uint32_t)bufx[offset + 1] << 0);
}

// write a 32 bit value to the buffer
void setBuf32(uint8_t offset, uint32_t val)
{ 
	bufx[0 + offset] = (val >> 24) & 0xff;
	bufx[1 + offset] = (val >> 16) & 0xff;
	bufx[2 + offset] = (val >>  8) & 0xff;
	bufx[3 + offset] = (val >>  0) & 0xff;
}

// write a 16 bit value to the buffer
void setBuf16(uint8_t offset, uint16_t val)
{
	bufx[0 + offset]  = (val >> 8) & 0xff;
	bufx[1 + offset]  = (val >> 0) & 0xff;
}

/*
Grundlagen zu diesen Funktionen wurden der Webseite:
http://www.cs.waikato.ac.nz/~312/crc.txt
entnommen (A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS)

Algorithmus entsprechend CRC32 fuer Ethernet

Startwert FFFFFFFF, LSB zuerst
Im Ergebnis kommt MSB zuerst und alle Bits sind invertiert

Das Ergebnis wurde geprueft mit dem CRC-Calculator:
http://www.zorc.breitbandkatze.de/crc.html
(Einstellung Button CRC-32 waehlen, Daten eingeben und calculate druecken)

Autor: K.Moraw, www.helitron.de, Oktober 2009
*/

static uint32_t reg32; // shift register

static uint32_t crc32_bytecalc(uint8_t byte)
{
	uint8_t i;
	uint32_t polynom = 0xEDB88320;		// Generatorpolynom

    for (i = 0; i < 8; ++i)
	{
        if ((reg32&1) != (byte&1))
             reg32 = (reg32>>1)^polynom; 
        else 
             reg32 >>= 1;
		byte >>= 1;
	}
	return reg32 ^ 0xffffffff;	 		// inverses Ergebnis, MSB zuerst
}

uint32_t crc32(uint8_t *data, uint8_t len)
{
	uint8_t i;
	reg32 = 0xffffffff;

	for (i = 0; i < len; i++)
	{
		crc32_bytecalc(data[i]);		// Berechne fuer jeweils 8 Bit der Nachricht
	}
	
	return reg32 ^ 0xffffffff;
}
