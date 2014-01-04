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

#include "lm75.h"

#include "config.h"
#include "i2c.h"
#include "util.h"

#ifdef LM92_TYPE
	#define CONV_TIME		1000		// uint16_t
	#define MASK_TEMP_L		0xf0
	#define CMD_SHUTDOWN	0x01
	#define CMD_WAKEUP		0x00   
#endif
#ifdef DS7505_TYPE
	#define CONV_TIME		200		// 12 bit Mode
	#define MASK_TEMP_L		0xf0
	#define CMD_SHUTDOWN	0x01
	#define CMD_WAKEUP		0x60		// wake up in 12 bit Mode
#endif

#define LM75_TEMP_REG		0x0
#define LM75_CONF_REG		0x1

//-----------------------------------------------------------------------------
void lm75_init(void)
{
	i2cInit();
}

int16_t lm75_get_tmp(void)
{
	int32_t temp;
	uint8_t temp_raw[2];
	
	// Read two bytes from temp register
	i2cReceive(LM75_I2C_ADR, LM75_TEMP_REG, 2, temp_raw);

#ifdef LM92_TYPE
	// Mask sign bit and put the two uint8 together
	temp = ((temp_raw[0] & 0b01111111) << 8) | temp_raw[1];
	// Divide by 8
	temp = temp >> 3;
#endif
#ifdef DS7505_TYPE
	// Mask sign bit and put the two uint8 together
	temp = ((temp_raw[0] & 0b01111111) << 8) | (temp_raw[1] & MASK_TEMP_L);
	// Divide by 8
	temp = temp >> 4;
#endif
	// Calc temp in 1/10000 Celsius
	temp = temp * 625;
	// Calc temp in centi Celsius;
	temp = temp / 100;

	if ((temp_raw[0] & 0b10000000) > 0)
	{
		// Temperature is negative
		temp = temp * -1;
	}
	return (temp);
}

void lm75_shutdown(void)
{
	uint8_t cmd[1];
	cmd[0]= CMD_SHUTDOWN;
	// Send the shutdown command to the config register
	i2cSend(LM75_I2C_ADR, LM75_CONF_REG, 1, cmd);
}

void lm75_wakeup(void)
{
	uint8_t cmd[1];
	cmd[0]= CMD_WAKEUP;
	// Send the wakeup command to the config register
	i2cSend(LM75_I2C_ADR, LM75_CONF_REG, 1, cmd);
}

uint16_t lm75_get_meas_time_ms(void)
{
	return(CONV_TIME);
}