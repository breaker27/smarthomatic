/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Stefan Baumann
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

#include "util.h"

/*
 * This driver is supposed to support many members of the LM75 familiy.
 * LM75 derivates like LM92, DS7505 and many other usually differ in conversion
 * time and accuracy/resolution
 *
 * Currently supported types: LM92 (tested)
 *							  DS7505 (tested)
 */
//#define LM92_TYPE		0		// LM92
#define DS7505_TYPE		0		// Maxim DS7505

/*
 * I2C_ADR is usually the first byte to be sent in an I2C command sequence.
 * bit[7:1] is used for I2C device adress
 * bit[0] contains a READ/WRITE flag
 * LM75_I2C_ADR contains the device adress not shifted
 */
#define LM75_I2C_ADR	0x90	// A0, A1 and A2 pin tied to GND

/*
 * Initialize device
 * Has to be called at least once before any other command
 */
void lm75_init(void);
/*
 * Reads temperature from lm75 or compatible.
 * Converts reading in centigrades.
 * The lm75 must not be in shutdown mode. If in doubt use lm75_wakeup().
 * Sufficient time has to be provided after lm75_wakeup() for the measurement
 * takes place. Please see datasheet
 * Returns: measure temperature in centigrades
 * 
 */
int16_t lm75_get_tmp(void);
/*
 * Puts device in powersaving shutdown mode
 */
void lm75_shutdown(void);
/*
 * Wakes device from shutdown mode and starts measurement
 */
void lm75_wakeup(void);
/*
 * Returns: The required time for a valid measurement
 */
uint16_t lm75_get_meas_time_ms(void);
