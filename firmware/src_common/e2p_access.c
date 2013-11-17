#include "e2p_access.h"

#ifdef UNITTEST

#include <stdio.h> // for using printf-debugging...

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

// Read UIntValue from EEPROM and limit it to the given boundaries.
// Go into error mode if bit length is not as assumed.
uint32_t _eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint16_t max_bits_for_type, uint8_t * array)
{
	if (length_bits > max_bits_for_type)
	{
		signal_error_state();
	}

	uint8_t x = 0;
	uint32_t val = 0;
	int8_t shift;
	
	while (length_bits + bit > x * 8)
	{
		shift = length_bits + bit - x * 8 - 8;
		uint32_t zz = (NULL == array) ? eeprom_read_byte((uint8_t*)(byte + x)) : array[byte + x];

		if (shift >= 0)
		{
			val += zz << shift;
		}
		else
		{
			val += zz >> - shift;
		}

		x++;
	}

	if (length_bits < 32)
	{
		val = val & ((1 << length_bits) - 1); // filter out only the wanted bits
	}

	if (val < minval)
	{
		val = minval;
	}
	
	if (val > maxval)
	{
		val = maxval;
	}

	return val;
}


// clear some bits within a byte
uint8_t clear_bits(uint8_t input, uint8_t bit, uint8_t bits_to_clear)
{
	uint8_t mask = ~((((1 << bits_to_clear) - 1)) << (8 - bits_to_clear - bit));
	return input & mask;
}

// get some bits from a 32 bit value, counted from the left (MSB) side! The first bit is bit nr. 0.
uint8_t get_bits(uint32_t input, uint8_t bit, uint8_t length)
{
	return (input >> (32 - length - bit)) & ((1 << length) - 1);
}

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

// write some bits to EEPROM only within one byte
void _eeprom_write_bits(uint16_t byte, uint8_t bit, uint16_t length, uint8_t val, uint8_t * array)
{
//	printf("Write value %d to byte %d bit %d with length %d\n", val, byte, bit, length);
	
	uint8_t b = 0;
	
	// if length is smaller than 8 bits, get the old value from eeprom
	if (length < 8)
	{
		b = (NULL == array) ? eeprom_read_byte((uint8_t*)(byte + 0)) : array[byte];	
		b = clear_bits(b, bit, length);
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

void _eeprom_write_UIntValue(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t val, uint8_t * array)
{
	// move bits to the left border
	val = val << (32 - length_bits);
		
	// 1st byte
	uint8_t src_start = 0;
	uint8_t dst_start = bit;
	uint8_t len = MIN(length_bits, 8 - bit);
	uint8_t val8 = get_bits(val, src_start, len);
	
	_eeprom_write_bits(byte, dst_start, len, val8, array);
	
	dst_start = 0;
	src_start = len;

	while (src_start < length_bits)
	{
		len = MIN(length_bits - src_start, 8);
		val8 = get_bits(val, src_start, len);
		byte++;

		//printf(" Byte nr. %d, src_start %d, len %d, val8 %d\n", byte, src_start, len, val8);

		_eeprom_write_bits(byte, dst_start, len, val8, array);
		
		src_start += len;
	}
}
