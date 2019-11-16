/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2019 Uwe Freese
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

#include "sps30.h"

#include "config.h"
#include "i2c.h"
#include "../src_common/uart.h" // for debugging only
#include "../src_common/util.h"


#define REG_CMD                     0x00
#define REG_RANGE_HIGH_BYTE         0x02
#define REG_RANGE_LOW_BYTE          0x03

#define CMD_START_MEASUREMENT                 0x0010
#define CMD_STOP_MEASUREMENT                  0x0104
#define CMD_READ_DATA_READY_FLAG              0x0202
#define CMD_READ_MEASURED_VALUES              0x0300
#define CMD_READ_WRITE_AUTO_CLEANING_INTERVAL 0x8004
#define CMD_START_FAN_CLEANING                0x5607
#define CMD_READ_ARTICLE_CODE                 0xD025
#define CMD_READ_SERIAL_NUMBER                0xD033
#define CMD_RESET                             0xD304

//-----------------------------------------------------------------------------

uint8_t calc_crc(uint8_t data[2])
{
	uint8_t crc = 0xFF;

	for(int i = 0; i < 2; i++)
	{
		crc ^= data[i];

		for (uint8_t bit = 8; bit > 0; --bit)
		{
			if (crc & 0x80)
			{
				crc = (crc << 1) ^ 0x31u;
			}
			else
			{
				crc = (crc << 1);
			}
		}
	}

	return crc;
}

void sps30_start_measurement(void)
{
	uint8_t data[2] = {0x03, 0x00};
	uint8_t crc = calc_crc(data);
	uint8_t cmd[5] = {CMD_START_MEASUREMENT >> 8, CMD_START_MEASUREMENT & 0xff, 0x03, 0x00, crc};
	i2c_write(SPS30_I2C_ADR, cmd, 5);
	i2c_stop();
}

void sps30_stop_measurement(void)
{
	uint8_t cmd[2] = {CMD_STOP_MEASUREMENT >> 8, CMD_STOP_MEASUREMENT & 0xff};
	i2c_write(SPS30_I2C_ADR, cmd, 2);
	i2c_stop();
}

bool sps30_read_data_ready(void)
{
	uint8_t cmd[2] = {CMD_READ_DATA_READY_FLAG >> 8, CMD_READ_DATA_READY_FLAG & 0xff};
	uint8_t data[3];

	i2c_write(SPS30_I2C_ADR, cmd, 2);

	// Device needs i2c_stop and some dalay between ic2_write and i2c_read.
	// Otherwise, values were all 0xff.
	i2c_stop();
	_delay_ms(50);

	i2c_read(SPS30_I2C_ADR, data, 3);
	i2c_stop();

	uint8_t crc = calc_crc(data);

	return (crc == data[2]) && data[1];
}

uint8_t sps30_data[60];

// Read the measured values.
// Call after data is ready (check with sps30_read_data_ready).
bool sps30_read_measured_values(void)
{
	uint8_t cmd[2] = {CMD_READ_MEASURED_VALUES >> 8, CMD_READ_MEASURED_VALUES & 0xff};

	i2c_write(SPS30_I2C_ADR, cmd, 2);

	// Device needs i2c_stop and some dalay between ic2_write and i2c_read.
	// Otherwise, values were all 0xff.
	i2c_stop();
	_delay_ms(50);

	i2c_read(SPS30_I2C_ADR, sps30_data, 60);
	i2c_stop();

	// Check CRCs. If wrong, abort.
	for (uint8_t i = 0; i < 60; i += 3)
	{
		if (calc_crc(&sps30_data[i]) != sps30_data[i + 2])
		{
			return false;
		}
	}

	return true;
}

float sps30_get_measured_value(uint8_t pos)
{
	float2uint32.uint32Val =
		(((uint32_t)sps30_data[pos * 6]) << 24)
		| (((uint32_t)sps30_data[pos * 6 + 1]) << 16)
		| (((uint32_t)sps30_data[pos * 6 + 3]) << 8)
		| (((uint32_t)sps30_data[pos * 6 + 4]) << 0);

	return float2uint32.floatVal;
}
