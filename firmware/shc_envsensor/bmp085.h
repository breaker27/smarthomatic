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

#include "../src_common/util.h"

/*
 * This driver is supposed to support the Bosch BMP085 barometric pressure
 * sensor
 */

/*
 * Initialize device
 * Has to be called at least once before any other command
 */
void bmp085_init(void);
/*
 * Measures and reads barometric pressure from BMP085 in pascal.
 * A measurement takes at most around 25 ms.
 * Returns: measured barometric pressure in pascal
 * 
 */
int32_t bmp085_meas_pressure(void);
/*
 * Measures and reads temperature from BMP085.
 * Converts reading in centigrades.
 * Returns: measured temperature in centigrades
 */
int16_t bmp085_meas_temp(void);
