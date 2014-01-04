/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Stefan Baumann
* 
* Based on i2csw.c from Pascal Stang (Original header included below)
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

/*! \file i2csw.c \brief Software I2C interface using port pins. */
//*****************************************************************************
//
// File Name	: 'i2csw.c'
// Title		: Software I2C interface using port pins
// Author		: Pascal Stang
// Created		: 11/22/2000
// Revised		: 5/2/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <avr/io.h>

#include "i2c.h"

#include "util.h"

// Standard I2C bit rates are:
// 100KHz for slow speed
// 400KHz for high speed

#define QDEL	_delay_us(2);		// i2c quarter-bit delay
#define HDEL	_delay_us(5);		// i2c half-bit delay

// i2c quarter-bit delay
// #define QDEL	asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop");
// i2c half-bit delay
// #define HDEL	asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop");

#define I2C_SDL_LO      cbi(SDAPORT, SDA)
#define I2C_SDL_HI      sbi(SDAPORT, SDA)

#define I2C_SCL_LO      cbi(SCLPORT, SCL); 
#define I2C_SCL_HI      sbi(SCLPORT, SCL); 

#define I2C_SCL_TOGGLE  HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
#define I2C_START       I2C_SDL_LO; QDEL; I2C_SCL_LO; 
#define I2C_STOP        HDEL; I2C_SCL_HI; QDEL; I2C_SDL_HI; HDEL;

/*
void i2ct(void)
{
	HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO;
}

void i2cstart(void)
{
	I2C_SDL_LO; QDEL; I2C_SCL_LO; 
}

void i2cstop(void)
{
	HDEL; I2C_SCL_HI; QDEL; I2C_SDL_HI; HDEL;
}


#define I2C_SCL_TOGGLE  i2ct();
#define I2C_START       i2cstart();
#define I2C_STOP        i2cstop();	
*/

uint8_t i2cPutbyte(uint8_t b)
{
	int8_t i;
	
	for (i=7;i>=0;i--)
	{
		if ( b & (1<<i) )
			I2C_SDL_HI;
		else
			I2C_SDL_LO;			// address bit
			I2C_SCL_TOGGLE;		// clock HI, delay, then LO
	}

	I2C_SDL_HI;					// leave SDL HI
	// added    
	cbi(SDADDR, SDA);			// change direction to input on SDA line (may not be needed)
	HDEL;
	I2C_SCL_HI;					// clock back up
  	b = (SDAPIN) & (1<<SDA);	// get the ACK bit

	HDEL;
	I2C_SCL_LO;					// not really ??
	sbi(SDADDR, SDA);			// change direction back to output
	HDEL;
	return (b == 0);			// return ACK value
}


uint8_t i2cGetbyte(uint8_t last)
{
	int8_t i;
	uint8_t c,b = 0;
		
	I2C_SDL_HI;					// make sure pullups are activated
	cbi(SDADDR, SDA);			// change direction to input on SDA line (may not be needed)

	for(i=7;i>=0;i--)
	{
		HDEL;
		I2C_SCL_HI;				// clock HI
	  	c = (SDAPIN) & (1<<SDA);  
		b <<= 1;
		if(c) b |= 1;
		HDEL;
    	I2C_SCL_LO;				// clock LO
	}

	sbi(SDADDR, SDA);			// change direction to output on SDA line
  
	if (last)
		I2C_SDL_HI;				// set NAK
	else
		I2C_SDL_LO;				// set ACK

	I2C_SCL_TOGGLE;				// clock pulse
	I2C_SDL_HI;					// leave with SDL HI
	return b;					// return received byte
}


//************************
//* I2C public functions *
//************************

//! Initialize I2C communication
void i2cInit(void)
{
	sbi(SDADDR, SDA);			// set SDA as output
	sbi(SCLDDR, SCL);			// set SCL as output
	I2C_SDL_HI;					// set I/O state and pull-ups
	I2C_SCL_HI;					// set I/O state and pull-ups
}

//! Send a byte sequence on the I2C bus
void i2cSend(uint8_t device, uint8_t subAddr, uint8_t length, uint8_t *data)
{
	I2C_START;      			// do start transition
	i2cPutbyte(device); 		// send DEVICE address
	i2cPutbyte(subAddr);		// and the subaddress

	// send the data
	while (length--)
		i2cPutbyte(*data++);

	I2C_SDL_LO;					// clear data line and
	I2C_STOP;					// send STOP transition
}

//! Retrieve a byte sequence on the I2C bus
void i2cReceive(uint8_t device, uint8_t subAddr, uint8_t length, uint8_t *data)
{
	uint8_t j = length;
	uint8_t *p = data;

	I2C_START;					// do start transition
	i2cPutbyte(device);			// send DEVICE address
	i2cPutbyte(subAddr);   		// and the subaddress
	HDEL;
	I2C_SCL_HI;      			// do a repeated START
	I2C_START;					// transition

	i2cPutbyte(device | READ);	// resend DEVICE, with READ bit set

	// receive data bytes
	while (j--)
		*p++ = i2cGetbyte(j == 0);

	I2C_SDL_LO;					// clear data line and
	I2C_STOP;					// send STOP transition
}

