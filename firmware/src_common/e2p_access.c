#include "e2p_access.h"

#ifdef UNITTEST

const uint8_t e2p[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255, 254, 253, 252, 251, 250, 249, 248 };

uint8_t eeprom_read_byte (const uint8_t *__p)
{
	uint32_t p = (uint32_t)__p;
	return e2p[p];
}

void signal_error_state(void)
{
	//exit(1);
}

#endif

// Read UIntValue from EEPROM and limit it to the given boundaries.
// Go into error mode if bit length is not as assumed.
uint32_t __eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint16_t max_bits_for_type)
{
	if (length_bits > max_bits_for_type)
	{
		signal_error_state();
	}
	
	uint32_t val = eeprom_read_byte((uint8_t*)(byte + 0)) << (24 + bit);
	
	if (length_bits + bit > 8)
	{
		val += eeprom_read_byte((uint8_t*)(byte + 1)) << (16 + bit);
	}
	
	if (length_bits + bit > 16)
	{
		val += eeprom_read_byte((uint8_t*)(byte + 2)) << (8 + bit);
	}
	
	if (length_bits + bit > 24)
	{
		val += eeprom_read_byte((uint8_t*)(byte + 3)) << (0 + bit);
	}
	
	if (length_bits + bit > 32)
	{
		val += eeprom_read_byte((uint8_t*)(byte + 4)) >> (8 - bit);
	}
	
	val = val >> (32 - length_bits); // move wanted bits to the lowest position

	if (length_bits < 32)
	{
		val = val & ((1 << length_bits) - 1);  // filter out only the wanted bits
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

// Wrapper function that casts to needed type and tells the internal function the max bit size of the type.
uint8_t eeprom_read_UIntValue8(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval)
{
	return (uint8_t)__eeprom_read_UIntValue32(byte, bit, length_bits, minval, maxval, 8);
}

// Wrapper function that casts to needed type and tells the internal function the max bit size of the type.
uint16_t eeprom_read_UIntValue16(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval)
{
	return (uint16_t)__eeprom_read_UIntValue32(byte, bit, length_bits, minval, maxval, 16);
}

// Wrapper function that casts to needed type and tells the internal function the max bit size of the type.
uint32_t eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval)
{
	return __eeprom_read_UIntValue32(byte, bit, length_bits, minval, maxval, 32);
}
