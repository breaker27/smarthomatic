/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Stefan Baumann, Uwe Freese
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

#ifndef _LM75_H
#define _LM75_H

#include "../src_common/util.h"

/*
 * This driver is supposed to support many members of the LM75 familiy.
 * LM75 derivates like LM92, DS7505 and many other usually differ in conversion
 * time and accuracy/resolution.
 *
 * Currently supported types: LM92 (tested)
 *							  DS7505 (tested)
 */
//#define LM92_TYPE     0       // LM92
#define DS7505_TYPE     0       // Maxim DS7505

/*
 * LM75_I2C_ADR is usually the first byte to be sent in an I2C command sequence,
 * shifter by one bit to the left and including a read/write bit as bit 0.
 * The value here is not shifted.l
 */
#define LM75_I2C_ADR    0x48    // A0, A1 and A2 pin tied to GND

#ifdef LM92_TYPE
	#define CONV_TIME       1000  // uint16_t
#endif
#ifdef DS7505_TYPE
	#define CONV_TIME       200   // 200ms at 12 bit Mode
#endif

/*
 * Wakes device from shutdown mode and starts measurement.
 */
void lm75_wakeup(void);

/*
 * Puts device in powersaving shutdown mode.
 */
void lm75_shutdown(void);

/*
 * Returns the required time to wait after wakeup before a valid measurement
 * can be read with lm75_get_tmp.
 */
static inline uint16_t lm75_get_meas_time_ms(void)
{
	return CONV_TIME;
}

/*
 * Reads temperature from lm75 or compatible.
 * Converts reading in centigrades.
 * The lm75 must not be in shutdown mode. If in doubt use lm75_wakeup().
 * Sufficient time has to be provided after lm75_wakeup() for the measurement
 * takes place. Please see datasheet.
 * Returns the measured temperature in centigrades.
 */
int16_t lm75_get_tmp(void);

#endif /* _LM75_H */
