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

#ifndef _UTIL_WATCHDOG_H
#define _UTIL_WATCHDOG_H

#define SRF_I2C_ADR	0x70

/*
 * Starts a ultrasonic range measurement and returns the result in
 * centimeters
 * Returns: distance in centimeters
 * 
 */
void rfm_watchdog_init(uint16_t deviceid, uint16_t timeout_sec);
void rfm_watchdog_reset(void);
void rfm_watchdog_count(uint16_t ms);

#endif /* _UTIL_WATCHDOG_H */
