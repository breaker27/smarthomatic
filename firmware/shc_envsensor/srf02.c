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

#include "srf02.h"

#include "config.h"
#include "i2c.h"
//#include "uart.h"
#include "../src_common/util.h"


#define REG_CMD                     0x00
#define REG_RANGE_HIGH_BYTE         0x02
#define REG_RANGE_LOW_BYTE          0x03

#define CMD_MEAS_REAL_INCHES        0x50
#define CMD_MEAS_REAL_CENTIMETER    0x51
#define CMD_MEAS_REAL_USEC          0x52

#define CMD_MEAS_FAKE_INCHES        0x56
#define CMD_MEAS_FAKE_CENTIMETER    0x57
#define CMD_MEAS_FAKE_USEC          0x58

#define MEAS_TIME_MS                70

//-----------------------------------------------------------------------------

uint16_t srf02_get_distance(void)
{
    uint8_t cmd[2] = {REG_CMD, CMD_MEAS_REAL_CENTIMETER};
    uint8_t data[2];
    uint16_t result;

    // Start measurement, wait and get result
    i2c_write(SRF_I2C_ADR, cmd, 2);
    i2c_stop();

    _delay_ms(MEAS_TIME_MS);

    cmd[0] = REG_RANGE_HIGH_BYTE;
    i2c_write(SRF_I2C_ADR, cmd, 1);
    i2c_read(SRF_I2C_ADR, data, 2);
    i2c_stop();

    result = ((data[0] << 8) + data[1]);
    //UART_PUTF("\r\nHighbyte: %u ", result);

    return result;
}
