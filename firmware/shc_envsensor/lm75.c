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

#include "lm75.h"

#include "i2c.h"
#include "../src_common/e2p_access.h"

#ifdef LM92_TYPE
	#define MASK_TEMP_L     0xf0
	#define CMD_SHUTDOWN    0x01
	#define CMD_WAKEUP      0x00   
#endif
#ifdef DS7505_TYPE
	#define MASK_TEMP_L     0xf0
	#define CMD_SHUTDOWN    0x01
	#define CMD_WAKEUP      0x60  // wake up in 12 bit Mode
#endif

#define LM75_TEMP_REG		0x0
#define LM75_CONF_REG		0x1

/*****************************************************************************
 * Private functions
 *****************************************************************************/

/*
 * Send an I2C command to the LM75 device using the config register.
 */
void lm75_send_cmd(uint8_t cmd)
{
	uint8_t data[2];
	data[0] = LM75_CONF_REG;
	data[1] = cmd;
	
	i2c_write(LM75_I2C_ADR, data, 2);
	i2c_stop();
}

/*
 * Receive I2C data from the given register.
 */
void lm75_receive(uint8_t reg, uint8_t length, uint8_t *data)
{
	i2c_write(LM75_I2C_ADR, &reg, 1);
	i2c_read(LM75_I2C_ADR, data, length);
	i2c_stop();
}

/*****************************************************************************
 * Public functions
 *****************************************************************************/
 
int16_t lm75_get_tmp(void)
{
	int32_t temp;
	uint8_t temp_raw[2];
	
	// Read two bytes from temp register
	lm75_receive(LM75_TEMP_REG, 2, temp_raw);

#ifdef LM92_TYPE
	// 13 bit signed value shifted to the left in two's complement
	temp = array_read_IntValue32(0, 13, -4096, 4095, temp_raw);
#endif
#ifdef DS7505_TYPE
	// 12 bit signed value shifted to the left in two's complement
	temp = array_read_IntValue32(0, 12, -2048, 2047, temp_raw);
#endif
	// Calc temp in 1/10000 Celsius
	temp = temp * 625;
	// Calc temp in centi Celsius;
	temp = temp / 100;

	return temp;
}

void lm75_shutdown(void)
{
	lm75_send_cmd(CMD_SHUTDOWN);
}

void lm75_wakeup(void)
{
	lm75_send_cmd(CMD_WAKEUP);
}
