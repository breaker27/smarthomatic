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

#ifndef _UTIL_HW_H
#define _UTIL_HW_H

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "e2p_hardware.h"

#define sbi(ADDRESS,BIT) ((ADDRESS) |= (1<<(BIT)))
#define cbi(ADDRESS,BIT) ((ADDRESS) &= ~(1<<(BIT)))

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

uint32_t packetcounter;

uint16_t bat_percentage(uint16_t vbat, uint16_t vempty);

void adc_init(void);
void adc_on(bool on);
uint16_t read_adc(uint8_t adc_input);
uint16_t read_battery(void);

void util_init(void);
void led_dbg(uint8_t ms);
void switch_led(bool b_on);
void led_blink(uint16_t on, uint16_t off, uint8_t times);
void check_eeprom_compatibility(DeviceTypeEnum deviceType);
void osccal_info(void);
void osccal_init(void);
void inc_packetcounter(void);
void rfm12_send_bufx(void);
void power_down(bool bod_disable);

#endif /* _UTIL_HW_H */
