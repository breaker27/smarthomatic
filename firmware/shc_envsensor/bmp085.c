/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Stefan Baumann
*
* adapted from Linux/drivers/misc/bmp085.c
* Copyright (c) 2010  Christoph Mair <christoph.mair@gmail.com>
* Copyright (c) 2012  Bosch Sensortec GmbH
* Copyright (c) 2012  Unixphere AB
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

#include "bmp085.h"

#include "config.h"
#include "i2c.h"
//#include "uart.h"
#include "util.h"


#define BMP085_I2C_ADR					0xEE   /* 0xEF (read) 0xEE (write) */
#define BMP085_CALIBRATION_DATA_START	0xAA
#define BMP085_CTRL_REG					0xF4
#define BMP085_TEMP_MEASUREMENT			0x2E
#define BMP085_PRESSURE_MEASUREMENT		0x34
#define BMP085_CONVERSION_REGISTER_MSB	0xF6
#define BMP085_CONVERSION_REGISTER_LSB	0xF7
#define BMP085_CONVERSION_REGISTER_XLSB 0xF8
#define BMP085_TEMP_CONVERSION_TIME		5
#define BMP085_OVERSAMPLING				2

struct bmp085_calibration_data {
	int16_t AC1, AC2, AC3;
	uint16_t AC4, AC5, AC6;
	int16_t B1, B2;
	int16_t MB, MC, MD;
};

struct bmp085_calibration_data cali;
uint32_t raw_temp;
uint32_t raw_pressure;
int32_t b6;				 /* calculated temperature correction coefficient */

void bmp085_update_raw_temp(void)
{
	uint8_t cmd[1];
	uint8_t data_raw[2];

	// Write command to ctrl register to start temp measurement
	cmd[0]= BMP085_TEMP_MEASUREMENT;
	i2cSend(BMP085_I2C_ADR, BMP085_CTRL_REG, 1, cmd);

	_delay_ms(BMP085_TEMP_CONVERSION_TIME);

	// Read temperature data from conversion register
	i2cReceive(BMP085_I2C_ADR, BMP085_CONVERSION_REGISTER_MSB, 2, data_raw);

	raw_temp = data_raw[0] << 8 | data_raw[1];

	return;
}

void bmp085_update_raw_pressure(void)
{
	uint8_t cmd[1];
	uint8_t data_raw[3];

	// Write command to ctrl register to start pressure measurement
	cmd[0]= BMP085_PRESSURE_MEASUREMENT + (BMP085_OVERSAMPLING << 6);
	i2cSend(BMP085_I2C_ADR, BMP085_CTRL_REG, 1, cmd);

	_delay_ms(2 + (3 << BMP085_OVERSAMPLING));

	// Read pressure data from conversion register
	i2cReceive(BMP085_I2C_ADR, BMP085_CONVERSION_REGISTER_MSB, 3, data_raw);

	raw_pressure = ((uint32_t)data_raw[0] << 16 |
					(uint32_t)data_raw[1] << 8 |
					 data_raw[2]) >> (8 - BMP085_OVERSAMPLING);

	return;
}

int32_t bmp085_get_temp(void)
{
	int32_t x1, x2, t;

	x1 = ((raw_temp - cali.AC6) * cali.AC5) >> 15;
	x2 = ((int32_t)cali.MC << 11) / (x1 + cali.MD);

	// Update b6 for pressure measurement
	b6 = x1 + x2 - 4000;
	t = (x1 + x2 + 8) >> 4;
	//UART_PUTF("x1: %d \r\n", x1);
	//UART_PUTF("x2: %d \r\n", x2);
	//UART_PUTF("b6: %d \r\n", b6);
	//UART_PUTF("temperature: %d \r\n", t);
	return t;
}

int32_t bmp085_get_pressure(void)
{
	int32_t x1, x2, x3, b3;
	uint32_t b4, b7;
	int32_t p;

	//UART_PUTF("b6: %d \r\n", b6);
	x1 = ((int32_t)b6 * b6) >> 12;
	x1 *= cali.B2;
	x1 >>= 11;

	x2 = (int32_t)cali.AC2 * b6;
	x2 >>= 11;

	x3 = x1 + x2;

	b3 = (((((int32_t)cali.AC1) * 4 + x3) << BMP085_OVERSAMPLING) + 2);

	b3 >>= 2;

	x1 = ((int32_t)cali.AC3 * b6) >> 13;
	x2 = ((int32_t)cali.B1 * (((int32_t)b6 * b6) >> 12)) >> 16;
	x3 = (x1 + x2 + 2) >> 2;
	b4 = ((uint32_t)cali.AC4 * (uint32_t)(x3 + 32768)) >> 15;

	b7 = ((uint32_t)raw_pressure - b3) * (50000 >> BMP085_OVERSAMPLING);
	p = ((b7 < 0x80000000) ? ((b7 << 1) / b4) : ((b7 / b4) * 2));

	x1 = p >> 8;
	x1 *= x1;
	x1 = (x1 * 3038) >> 16; //OK
	x2 = (-7357 * p) >> 16;
	p += (x1 + x2 + 3791) >> 4;
	return p;
}

//-----------------------------------------------------------------------------
void bmp085_init(void)
{
	uint8_t data_raw[22];

	i2cInit();

	i2cReceive(BMP085_I2C_ADR, BMP085_CALIBRATION_DATA_START, 22, data_raw);

	cali.AC1 = data_raw[0] << 8 | data_raw[1];
	cali.AC2 = data_raw[2] << 8 | data_raw[3];
	cali.AC3 = data_raw[4] << 8 | data_raw[5];
	cali.AC4 = data_raw[6] << 8 | data_raw[7];
	cali.AC5 = data_raw[8] << 8 | data_raw[9];
	cali.AC6 = data_raw[10] << 8 | data_raw[11];
	cali.B1 = data_raw[12] << 8 | data_raw[13];
	cali.B2 = data_raw[14] << 8 | data_raw[15];
	cali.MB = data_raw[16] << 8 | data_raw[17];
	cali.MC = data_raw[18] << 8 | data_raw[19];
	cali.MD = data_raw[20] << 8 | data_raw[21];
	
	return;
}

int32_t bmp085_meas_pressure(void)
{
	bmp085_update_raw_temp();
	bmp085_update_raw_pressure();
	return bmp085_get_pressure();
}

int16_t bmp085_meas_temp(void)
{
	bmp085_update_raw_temp();
	return (int16_t)(bmp085_get_temp() * 10);
}

