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

#include "htu21d.h"

#include "config.h"
#include "i2c.h"
#include "../src_common/uart.h"
#include "../src_common/util.h"

#define HTU21D_I2C_ADR              0x40
#define CMD_MEAS_HUMIDITY           0xe5
#define CMD_MEAS_TEMPERATURE        0xe3

#define MEAS_TIME_MS                50

//-----------------------------------------------------------------------------

uint16_t htu21d_meas_raw(uint8_t type)
{
	uint8_t cmd[1] = {type};
	uint8_t data[3];

	// Start measurement, wait and get result
	i2c_write(HTU21D_I2C_ADR, cmd, 1);
	i2c_stop();

	_delay_ms(MEAS_TIME_MS);

	i2c_read(HTU21D_I2C_ADR, data, 3);
	i2c_stop();

	/*UART_PUTF("htu21d Byte 0: %u, ", data[0]);
	UART_PUTF("Byte 1: %u, ", data[1]);
	UART_PUTF("Byte 2: %u\r\n", data[2]);*/

	return ((uint16_t)data[0] << 8) + (data[1] && 0b11111100); // remove status bits
}

uint16_t htu21d_meas_hum(void)
{
	uint16_t raw = htu21d_meas_raw(CMD_MEAS_HUMIDITY);
	uint16_t result = (uint16_t)((int32_t)125 * 100 * raw / 65536 - 6 * 100);
	return result;
}

int16_t htu21d_meas_temp(void)
{
	uint16_t raw = htu21d_meas_raw(CMD_MEAS_TEMPERATURE);
	int16_t result = (int32_t)17572 * raw / 65536 - 4685;
	return result;
}
