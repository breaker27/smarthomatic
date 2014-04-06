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

#include "i2c.h"

#include <stddef.h>
#include <stdint.h>
#include <avr/io.h>

/*****************************************************************************
 * Private constants
 *****************************************************************************/

/**
 * Mask for TWSR status bits
 */
#define TWI_STATUS_MASK		((1 << TWS3) | (1 << TWS4) | (1 << TWS5) | (1 << TWS6) | (1 << TWS7))

/**
 * TWSR status codes
 */
typedef enum
{
	TWI_STATUS_START		= 0x08,
	TWI_STATUS_RSTART		= 0x10,
	TWI_STATUS_SLAW_ACK		= 0x18,
	TWI_STATUS_SLAW_NOACK	= 0x20,
	TWI_STATUS_WRITE_ACK	= 0x28,
	TWI_STATUS_WRITE_NOACK	= 0x30,
	TWI_STATUS_ARBLOST		= 0x38,
	TWI_STATUS_SLAR_ACK		= 0x40,
	TWI_STATUS_SLAR_NOACK	= 0x48,
	TWI_STATUS_READ_ACK		= 0x50,
	TWI_STATUS_READ_NOACK	= 0x58
} TWI_STATUS_t;

/**
 * Calculate TWBR value from I2C clock and prescaler.
 * @param	i2c		I2C clock
 * @param	pre		Prescaler value
 * @return	TWBR value
 */
#define TWI_CALCULATE_TWBR(i2c,pre) (uint8_t)((F_CPU / i2c - 16) / (2 * pre))

/**
 * TWI prescaler configuration.
 */
typedef enum
{
	TWI_PRESCALER_1		= (0 << TWPS1) | (0 << TWPS0),
	TWI_PRESCALER_4		= (0 << TWPS1) | (1 << TWPS0),
	TWI_PRESCALER_16	= (1 << TWPS1) | (0 << TWPS0),
	TWI_PRESCALER_64	= (1 << TWPS1) | (1 << TWPS0)
} TWI_PRESCALER_t;

#define I2C_CFG_BITRATE      50000UL            // 50 kHz is maximum bitrate for 1 MHz clock
#define I2C_CFG_PRESCALER    TWI_PRESCALER_1

/**
 * Transfer direction for address
 */
typedef enum
{
	TWI_DIR_WRITE	= 0x00,
	TWI_DIR_READ	= 0x01
} TWI_DIR_t;


/*****************************************************************************
 * Private functions
 *****************************************************************************/

/**
 * Wait for TWINT
 */
void i2c_wait(void) {
	while (!(TWCR & (1 << TWINT)))
		;
}

/**
 * Write (RE-)START condition and address.
 * @param	addr	Slave address
 * @param	dir		Read/Write
 * @return	0 if OK, -1 if error
 */
uint8_t i2c_writeAddr(const uint8_t addr, const TWI_DIR_t dir)
{
	// send (RE-)START condition
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	i2c_wait();
	if (!(TWSR & (TWI_STATUS_START | TWI_STATUS_RSTART)))
		return -1;

	// send SLA+W/R
	TWDR = (addr << 1) | dir;
	TWCR = (1 << TWINT) | (1 << TWEN);
	i2c_wait();
	if (dir == TWI_DIR_WRITE) {
		if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_SLAW_ACK)
			return -1;
	} else {
		if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_SLAR_ACK)
			return -1;
	}

	return 0;
}

/*****************************************************************************
 * I2C interface members
 *****************************************************************************/
 
void i2c_enable()
{
	// disable TWI
	TWCR = 0x00;
	// setup clock
	TWBR = TWI_CALCULATE_TWBR(I2C_CFG_BITRATE, 1);
	TWSR = I2C_CFG_PRESCALER;
	// no address for master
	TWAR = 0x00;
	// enable
	TWCR = (1 << TWEN);
}

void i2c_disable()
{
	// disable TWI
	TWCR = 0x00;
}

uint8_t i2c_write(const uint8_t addr, const uint8_t *buf, uint8_t len)
{
	uint8_t sent = 0;

	// send (RE-)START and address
	if (i2c_writeAddr(addr, TWI_DIR_WRITE))
		goto error;

	// send data
	while (sent < len)
	{
		TWDR = *buf;
		TWCR = (1 << TWINT) | (1 << TWEN);
		i2c_wait();
		if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_WRITE_ACK)
			goto error;
		++sent;
		++buf;
	}

error:
	return sent;
}

uint8_t i2c_read(const uint8_t addr, uint8_t *buf, uint8_t len)
{
	uint8_t read = 0;

	// send (RE-)START and address
	if (i2c_writeAddr(addr, TWI_DIR_READ))
		goto error;

	// read data
	while (read < len)
	{
		uint8_t ack = (read == len-1) ? (0 << TWEA) : (1 << TWEA);
		TWCR = (1 << TWINT) | (1 << TWEN) | ack;
		i2c_wait();
		if ((TWSR & TWI_STATUS_MASK) == TWI_STATUS_ARBLOST)
			goto error;
		*buf = TWDR;
		++read;
		++buf;
	}

error:
	return read;
}

void i2c_stop()
{
	// send STOP
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}
