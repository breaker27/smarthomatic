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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src_common/e2p_access.h"

uint8_t res = 0;

void compare(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value, uint32_t val)
{
	printf("Test eeprom_read_UIntValueXX(%d, %d, %d, %u, %u)\n", byte, bit, length_bits, minval, maxval);
	printf("  Assumed value: %d, returned value: %u", assumed_value, val);
	
	if (val != assumed_value)
	{
		res = 1;
		printf(" --> NOK\n");
	}
	else
	{
		printf(" --> OK\n");
	}
}

// Test one of the e2p read functions.
void test_eeprom_read_UIntValue8(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value)
{
	uint32_t x = eeprom_read_UIntValue8(byte, bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_eeprom_read_UIntValue16(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value)
{
	uint32_t x = eeprom_read_UIntValue16(byte, bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value)
{
	uint32_t x = eeprom_read_UIntValue32(byte, bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

int main(int argc , char** argv){
	printf("smarthomatic unit test\n");

	// 8 Bit tests
	test_eeprom_read_UIntValue8(5, 0, 8, 0, 255, 5); // normal read
	test_eeprom_read_UIntValue8(5, 0, 8, 7, 255, 7); // min test
	test_eeprom_read_UIntValue8(5, 0, 8, 0, 3, 3); // max test
	test_eeprom_read_UIntValue8(5, 1, 7, 0, 255, 5); // offset test within a byte
	test_eeprom_read_UIntValue8(5, 2, 8, 0, 255, 5 << 2); // offset test over byte boundary
	test_eeprom_read_UIntValue8(5, 7, 1, 0, 255, 1); // bit masking test
	
	// 16 Bit tests
	test_eeprom_read_UIntValue16(5, 0, 16, 0, 65535, 1286); // normal read
	test_eeprom_read_UIntValue16(5, 0, 16, 1400, 65535, 1400); // min test
	test_eeprom_read_UIntValue16(5, 0, 16, 0, 3, 3); // max test
	test_eeprom_read_UIntValue16(5, 1, 15, 0, 65535, 1286); // offset test within a byte
	test_eeprom_read_UIntValue16(5, 2, 16, 0, 65535, 1286 << 2); // offset test over byte boundary
	test_eeprom_read_UIntValue16(5, 7, 1, 0, 65535, 1); // bit masking test

	// 32 Bit tests
	test_eeprom_read_UIntValue32(5, 0, 32, 0, 4294967295U, 84281096U); // normal read
	test_eeprom_read_UIntValue32(5, 0, 32, 90000000U, 4294967295U, 90000000U); // min test
	test_eeprom_read_UIntValue32(5, 0, 32, 0, 3, 3); // max test
	test_eeprom_read_UIntValue32(5, 1, 31, 0, 4294967295U, 84281096U); // offset test within a byte
	test_eeprom_read_UIntValue32(5, 2, 32, 0, 4294967295U, 84281096U << 2); // offset test over byte boundary
	test_eeprom_read_UIntValue32(5, 7, 1, 0, 4294967295U, 1); // bit masking test

	return res;
}
