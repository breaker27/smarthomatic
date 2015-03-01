/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Uwe Freese
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
*
*/

/*
* This 1-wire lib defines some high level functions to access the DS18S20 and
* DS18B20 temperature sensors. It currently supports only one sensor.
*
* Internal low level functions which implement the communication according to
* the 1-wire protocol are in the *.c file.
*/

#ifndef _ONEWIRE_H
#define _ONEWIRE_H

#include <stdbool.h>
#include <stdint.h>

#define ONEWIRE_PORT PORTD
#define ONEWIRE_DDR DDRD
#define ONEWIRE_PIN 4
#define ONEWIRE_PINREG PIND

#define NO_TEMPERATURE 65535

// Initializes some pin states. Call once after startup.
extern void onewire_init(void);

// Writes the ROM ID (8 bytes) of the 1-wire slave to an array. The ID is
// used in further commands.
// Returns true if an error occurred (no slave found).
extern bool onewire_get_rom_id(uint8_t * id_array);

// Get temperature from the DS18S20/DS18B20 with the given ROM ID in 1/100°C.
// Returns NO_TEMPERATURE in case of an error.
extern int16_t onewire_get_temperature(uint8_t * id_array);

#endif /* _ONEWIRE_H */
