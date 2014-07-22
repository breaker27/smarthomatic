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

// Unit test program - runs on the PC, not the microcontroller.
// Tested under Windows with mingw.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "../src_common/e2p_access.h"
#include "../src_common/msggrp_weather.h"
#include "../src_common/msggrp_gpio.h"

uint8_t res = 0;

uint8_t testarray[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255, 254, 253, 252, 251, 250, 249, 248 };


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
	uint32_t x = eeprom_read_UIntValue8(byte * 8 + bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_eeprom_read_UIntValue16(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value)
{
	uint32_t x = eeprom_read_UIntValue16(byte * 8 + bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_eeprom_read_UIntValue32(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value)
{
	uint32_t x = eeprom_read_UIntValue32(byte * 8 + bit, length_bits, minval, maxval);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_array_read_UIntValue8(uint16_t byte, uint8_t bit, uint16_t length_bits, uint32_t minval, uint32_t maxval, uint32_t assumed_value, uint8_t * array)
{
	uint32_t x = array_read_UIntValue8(byte * 8 + bit, length_bits, minval, maxval, array);
	compare(byte, bit, length_bits, minval, maxval, assumed_value, x);
}

void test_eeprom_value(uint16_t pos, uint8_t assumed_value)
{
	uint8_t val = eeprom_read_byte((uint8_t*)(pos + 0));
	
	printf("Test eeprom value at pos %d. Assumed value: %d, returned value: %d", pos, assumed_value, val);
	
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

void test_array_value(uint16_t pos, uint8_t assumed_value)
{
	uint8_t val = testarray[pos];
	
	printf("Test array value at pos %d. Assumed value: %d, returned value: %d", pos, assumed_value, val);
	
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

void test_bufx(char * assumed_value)
{
	char s[100];

	uint8_t i;
	
	for (i = 0; i < 32; i++)
	{
		sprintf(s + i * 2, "%02x", bufx[i]);
	}
	
	printf("Test access functions for e2p / packet data\r\n");
	printf("  Assumed value: %s, returned value: %s", assumed_value, s);

	if (strcmp(assumed_value, s) != 0)
	{
		res = 1;
		printf(" --> NOK\n");
	}
	else
	{
		printf(" --> OK\n");
	}
}

int main(int argc , char** argv){
	printf("smarthomatic unit test\n");

	// 8 Bit read tests
	test_eeprom_read_UIntValue8(5, 0, 8, 0, 255, 5); // normal read
	test_eeprom_read_UIntValue8(5, 0, 8, 7, 255, 7); // min test
	test_eeprom_read_UIntValue8(5, 0, 8, 0, 3, 3); // max test
	test_eeprom_read_UIntValue8(5, 1, 7, 0, 255, 5); // offset test within a byte
	test_eeprom_read_UIntValue8(5, 2, 8, 0, 255, 5 << 2); // offset test over byte boundary
	test_eeprom_read_UIntValue8(5, 7, 1, 0, 255, 1); // bit masking test
	
	// 16 Bit read tests
	test_eeprom_read_UIntValue16(5, 0, 16, 0, 65535, 1286); // normal read
	test_eeprom_read_UIntValue16(5, 0, 16, 1400, 65535, 1400); // min test
	test_eeprom_read_UIntValue16(5, 0, 16, 0, 3, 3); // max test
	test_eeprom_read_UIntValue16(5, 1, 15, 0, 65535, 1286); // offset test within a byte
	test_eeprom_read_UIntValue16(5, 2, 16, 0, 65535, 1286 << 2); // offset test over byte boundary
	test_eeprom_read_UIntValue16(5, 7, 1, 0, 65535, 1); // bit masking test

	// 32 Bit read tests
	test_eeprom_read_UIntValue32(5, 0, 32, 0, 4294967295U, 84281096U); // normal read
	test_eeprom_read_UIntValue32(5, 0, 32, 90000000U, 4294967295U, 90000000U); // min test
	test_eeprom_read_UIntValue32(5, 0, 32, 0, 3, 3); // max test
	test_eeprom_read_UIntValue32(5, 1, 31, 0, 4294967295U, 84281096U); // offset test within a byte
	test_eeprom_read_UIntValue32(5, 2, 32, 0, 4294967295U, 84281096U << 2); // offset test over byte boundary
	//test_eeprom_read_UIntValue32(5, 7, 1, 0, 4294967295U, 1); // bit masking test

	// 8 Bit read tests (ARRAY)
	test_array_read_UIntValue8(5, 0, 8, 0, 255, 5, testarray); // normal read
	test_array_read_UIntValue8(5, 0, 8, 7, 255, 7, testarray); // min test
	test_array_read_UIntValue8(5, 0, 8, 0, 3, 3, testarray); // max test
	test_array_read_UIntValue8(5, 1, 7, 0, 255, 5, testarray); // offset test within a byte
	test_array_read_UIntValue8(5, 2, 8, 0, 255, 5 << 2, testarray); // offset test over byte boundary
	test_array_read_UIntValue8(5, 7, 1, 0, 255, 1, testarray); // bit masking test

	// write tests within one byte
	eeprom_write_UIntValue(5 * 8 + 0, 8, 133); // normal write
	test_eeprom_value(5, 133);
	eeprom_write_UIntValue(5 * 8 + 1, 2, 3); // offset test 1
	test_eeprom_value(5, 229);
	eeprom_write_UIntValue(5 * 8 + 2, 1, 0); // offset test 2
	test_eeprom_value(5, 197);

	// write test over byte boundary
	eeprom_write_UIntValue(4 * 8 + 4, 8, 0); // offset test 2
	test_eeprom_value(4, 0);
	test_eeprom_value(5, 5);

	// write test 32 bits over byte boundary
	eeprom_write_UIntValue(4 * 8 + 4, 32, 0b10101010101011001100110011011010); // offset test 3
	test_eeprom_value(4, 10);
	test_eeprom_value(5, 170);
	test_eeprom_value(6, 204);
	test_eeprom_value(7, 205);
	test_eeprom_value(8, 168);
	
	// write test 17 bits over byte boundary
	eeprom_write_UIntValue(16 * 8 + 3, 17, 0b11110000111100001); // offset test 3
	test_eeprom_value(16, 254);
	test_eeprom_value(17, 30);
	test_eeprom_value(18, 29);
	
	// write tests within one byte (ARRAY)
	array_write_UIntValue(5 * 8 + 0, 8, 133, testarray); // normal write
	test_array_value(5, 133);
	array_write_UIntValue(5 * 8 + 1, 2, 3, testarray); // offset test 1
	test_array_value(5, 229);
	array_write_UIntValue(5 * 8 + 2, 1, 0, testarray); // offset test 2
	test_array_value(5, 197);
	
	// write test envsensor
	pkg_header_init_weather_temperature_status();
	msg_weather_temperature_set_temperature(271);
	pkg_header_set_senderid(177);
	pkg_header_set_packetcounter(2345);
	test_bufx("000000000b10009298142021e000000000000000000000000000000000000000");
	
	// write test GPIO analogpin
	pkg_header_init_gpio_analogpin_status();
	msg_gpio_analogpin_set_on(1, true);
	msg_gpio_analogpin_set_on(3, true);
	msg_gpio_analogpin_set_on(4, true);
	msg_gpio_analogpin_set_voltage(0, 1100);
	msg_gpio_analogpin_set_voltage(1, 8);
	msg_gpio_analogpin_set_voltage(4, 0);
	pkg_header_set_senderid(999);
	pkg_header_set_packetcounter(65432);
	test_bufx("000000003e700ff988024b113004000000000000000000000000000000000000");
	
	printf("\r\nOverall result: %s\r\n", res ? "NOK (at least one test failed)" : "OK (all tests ok)");
	
	return res;
}
