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

#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#define sbi(ADDRESS,BIT) ((ADDRESS) |= (1<<(BIT)))
#define cbi(ADDRESS,BIT) ((ADDRESS) &= ~(1<<(BIT)))

#include "e2p_layout.h" // device specific (!) version of e2p layout in the device's directory

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

// used as buffer for sending data to SHT
uint8_t bufx[65];
unsigned int adc_data;
uint32_t packetcounter;

void print_signed(int16_t i);
void print_bytearray(uint8_t * b, uint8_t len);
uint16_t linear_interpolate16(uint16_t in, uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out);
uint32_t linear_interpolate32(uint32_t in, uint32_t min_in, uint32_t max_in, uint32_t min_out, uint32_t max_out);
float linear_interpolate_f(float in, float min_in, float max_in, float min_out, float max_out);
int bat_percentage(int vbat);

void adc_init(void);
void adc_on(bool on);
unsigned int read_adc(unsigned char adc_input);

unsigned long crc32(unsigned char *data, int len);

// read/write 16 bit / 32 bit values to the byte buffer in PC byte order
uint32_t getBuf16(uint8_t offset);
uint32_t getBuf32(uint8_t offset);
void setBuf32(uint8_t offset, uint32_t val);
void setBuf16(uint8_t offset, uint16_t val);

void util_init(void);
void switch_led(bool b_on);
void led_blink(uint16_t on, uint16_t off, uint8_t times);
void check_eeprom_compatibility(uint8_t deviceType);
void osccal_info(void);
void osccal_init(void);
void inc_packetcounter(void);
void rfm12_sendbuf(void);

#endif
