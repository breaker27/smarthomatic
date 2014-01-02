/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Stefan Baumann
* 
* Based on i2csw.h from Pascal Stang (Original header included below)
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

/*! \file i2csw.h \brief Software I2C interface using port pins. */
//*****************************************************************************
//
// File Name	: 'i2csw.h'
// Title		: Software I2C interface using port pins
// Author		: Pascal Stang
// Created		: 11/22/2000
// Revised		: 5/2/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
///	\ingroup driver_sw
/// \defgroup i2csw Software I2C Serial Interface Function Library (i2csw.c)
/// \code #include "i2csw.h" \endcode
/// \par Overview
///		This library provides a very simple bit-banged I2C serial interface.
/// The library supports MASTER mode send and receive of single or multiple
/// bytes.  Thanks to the standardization of the I2C protocol and register
/// access, the send and receive commands are everything you need to talk to
/// thousands of different I2C devices including: EEPROMS, Flash memory,
/// MP3 players, A/D and D/A converters, electronic potentiometers, etc.
///
/// Although some AVR processors have built-in hardware to help create an I2C
/// interface, this library does not require or use that hardware.
///
/// For general information about I2C, see the i2c library.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef I2C_H
#define I2C_H

#include "config.h"

// clock line port
#define SCLPORT		PORTC	// i2c clock port
#define SCLDDR		DDRC	// i2c clock port direction
// data line port
#define SDAPORT		PORTC	// i2c data port
#define SDADDR		DDRC	// i2c data port direction
#define SDAPIN		PINC	// i2c data port input
// pin assignments
#define SCL			4		// i2c clock pin
#define SDA			3		// i2c data pin

// defines and constants
#define READ		0x01	// I2C READ bit

// functions

// initialize I2C interface pins
void i2cInit(void);

// send I2C data to <device> register <sub>
void i2cSend(uint8_t device, uint8_t sub, uint8_t length, uint8_t *data);

// receive I2C data from <device> register <sub>
void i2cReceive(uint8_t device, uint8_t sub, uint8_t length, uint8_t *data);

#endif
