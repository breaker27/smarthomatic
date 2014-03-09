/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2014 Bernhard Stegmaier, Uwe Freese
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

#ifndef I2C_H_
#define I2C_H_

#include <stddef.h>
#include <avr/io.h>

/**
 * Enable function for I2C driver interface.
 */
void i2c_enable(void);

/**
 * Disable function for I2C driver interface.
 */
void i2c_disable(void);

/**
 * Write function for I2C driver interface.
 */
uint8_t i2c_write(const uint8_t addr, const uint8_t *buf, uint8_t len);

/**
 * Read function for I2C driver interface.
 */
uint8_t i2c_read(const uint8_t addr, uint8_t *buf, uint8_t len);

/**
 * Stop function for I2C driver interface.
 */
void i2c_stop(void);

#endif /* I2C_H_ */
