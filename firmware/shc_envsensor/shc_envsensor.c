/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include "rfm12.h"
#include "uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_weather.h"
#include "../src_common/msggrp_environment.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_envsensor.h"

#include "sht11.h"
#include "i2c.h"
#include "lm75.h"
#include "bmp085.h"

#include "aes256.h"
#include "util.h"
#include "version.h"

#define AVERAGE_COUNT 4 // Average over how many values before sending over RFM12?
#define AVERAGE_COUNT_BATT 120
#define AVERAGE_COUNT_VERSION 800

uint8_t temperature_sensor_type = 0;
uint8_t humidity_sensor_type = 0;
uint8_t barometric_sensor_type = 0;
uint8_t brightness_sensor_type = 0;
uint16_t version_status_cycle = AVERAGE_COUNT_VERSION - 1; // send promptly after startup
uint16_t vempty = 1100; // 1.1V * 2 cells = 2.2V = min. voltage for RFM2

struct measurement_t
{
	int32_t val; // stores the accumulated value
	uint8_t cnt; // amount of single measurements
	uint8_t avgThr; // amount of measurements that are averaged into one value to send 
} temperature, humidity, barometric_pressure, battery_voltage, brightness;

// ---------- functions to measure values from sensors ----------

void sht11_measure_loop(void)
{
	sht11_start_measure();
	_delay_ms(500);
	while (!sht11_measure_finish());
}

void measure_temperature(void)
{
	if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
	{
		sht11_measure_loop();
		temperature.val += sht11_get_tmp();
		temperature.cnt++;
	}
	else if (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
	{
		lm75_wakeup();
		_delay_ms(lm75_get_meas_time_ms());
		temperature.val += lm75_get_tmp();
		temperature.cnt++;
		lm75_shutdown();
	}
	else if (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085)
	{
		temperature.val += bmp085_meas_temp();
		temperature.cnt++;
	}
}

void measure_humidity(void)
{
	if (humidity_sensor_type == HUMIDITYSENSORTYPE_SHT15)
	{
		// actually measure if not done already while getting temperature
		if (temperature_sensor_type != TEMPERATURESENSORTYPE_SHT15)
		{
			sht11_measure_loop();
		}
	
		humidity.val += sht11_get_hum();
		humidity.cnt++;
	}
}

void measure_barometric_pressure(void)
{
	if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
	{
		barometric_pressure.val += bmp085_meas_pressure();
		barometric_pressure.cnt++;
	}
}

void measure_battery_voltage(void)
{
	battery_voltage.val += (int)((long)read_adc(0) * 34375 / 10000 / 2); // 1.1 * 480 Ohm / 150 Ohm / 1,024
	battery_voltage.cnt++;
}

void measure_brightness(void)
{
	if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
	{
		brightness.val += read_adc(1);
		brightness.cnt++;
	}
}

// ---------- functions to prepare a message filled with sensor data ----------

void prepare_humiditytemperature(void)
{
	humidity.val = humidity.val / humidity.avgThr; // in 100 * % rel.
	temperature.val /= temperature.avgThr;

	pkg_header_init_weather_humiditytemperature_status();
	msg_weather_humiditytemperature_set_humidity(humidity.val / 10); // in permill
	msg_weather_humiditytemperature_set_temperature(temperature.val);
	
	UART_PUTF2("Send humidity: %u.%u%%, temperature: ", humidity.val / 100, humidity.val % 100);
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");

	humidity.val = temperature.val = humidity.cnt = temperature.cnt = 0;
}

void prepare_barometricpressuretemperature(void)
{
	barometric_pressure.val /= barometric_pressure.avgThr;
	temperature.val /= temperature.avgThr;

	pkg_header_init_weather_barometricpressuretemperature_status();
	msg_weather_barometricpressuretemperature_set_barometricpressure(barometric_pressure.val);
	msg_weather_barometricpressuretemperature_set_temperature(temperature.val);

	UART_PUTF("Send barometric pressure: %ld pascal, temperature: ", barometric_pressure.val);
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");
	
	barometric_pressure.val = temperature.val = barometric_pressure.cnt = temperature.cnt = 0;
}

void prepare_temperature(void)
{
	pkg_header_init_weather_temperature_status();
	msg_weather_temperature_set_temperature(temperature.val / temperature.avgThr);
	
	UART_PUTS("Send temperature: ");
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");
	
	temperature.val = temperature.cnt = 0;
}

void prepare_brightness(void)
{
	brightness.val /= brightness.avgThr;
	brightness.val = 100 - (int)((long)brightness.val * 100 / 1024);

	pkg_header_init_environment_brightness_status();
	msg_environment_brightness_set_brightness(brightness.val);
	
	UART_PUTF("Send brightness: %u%%\r\n", brightness.val);
	
	brightness.val = brightness.cnt = 0;
}

void prepare_battery_voltage(void)
{
	battery_voltage.val /= battery_voltage.cnt;
	battery_voltage.val = bat_percentage(battery_voltage.val, vempty);

	pkg_header_init_generic_batterystatus_status();
	msg_generic_batterystatus_set_percentage(battery_voltage.val);
				
	UART_PUTF("Send battery: %u%%\r\n", battery_voltage.val);

	battery_voltage.val = battery_voltage.cnt = 0;
}

void prepare_version(void)
{
	// Set packet content
	pkg_header_init_generic_version_status();
	msg_generic_version_set_major(VERSION_MAJOR);
	msg_generic_version_set_minor(VERSION_MINOR);
	msg_generic_version_set_patch(VERSION_PATCH);
	msg_generic_version_set_hash(VERSION_HASH);

	UART_PUTF4("Send version: v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	version_status_cycle = 0;
}

// ---------- main loop ----------

int main(void)
{
	uint8_t device_id = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_ENVSENSOR);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device specific config
	temperature_sensor_type = e2p_envsensor_get_temperaturesensortype();
	humidity_sensor_type = e2p_envsensor_get_humiditysensortype();
	barometric_sensor_type = e2p_envsensor_get_barometricsensortype();
	brightness_sensor_type = e2p_envsensor_get_brightnesssensortype();

	// read device id
	device_id = e2p_generic_get_deviceid();
	
	osccal_init();
	
	uart_init();
	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic EnvSensor v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2012..2014 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	UART_PUTF ("Temperature sensor type: %u\r\n", temperature_sensor_type);
	UART_PUTF ("Humidity sensor Type: %u\r\n", humidity_sensor_type);
	UART_PUTF ("Barometric sensor Type: %u\r\n", barometric_sensor_type);
	UART_PUTF ("Brightness sensor type: %u\r\n", brightness_sensor_type);
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	// Different average counts per measurement would be possible.
	// For now, use the same fixed value for all measurements.
	temperature.avgThr = humidity.avgThr = barometric_pressure.avgThr = brightness.avgThr = AVERAGE_COUNT;
	battery_voltage.avgThr = AVERAGE_COUNT_BATT;

	adc_init();

	if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
	{
	  sht11_init();
	  vempty = 1200; // 1.2V * 2 cells = 2.4V = min. voltage for SHT15
	}
	
	UART_PUTF ("Min. battery voltage: %umV\r\n", vempty);

	if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
	{
		i2c_enable();
		bmp085_init();
		i2c_disable();
	}

	led_blink(500, 500, 3);
	
	rfm12_init();
	
	//rfm12_set_wakeup_timer(0b11100110000);   // ~ 6s
	//rfm12_set_wakeup_timer(0b11111000000);   // ~ 24576ms
	//rfm12_set_wakeup_timer(0b0100101110101); // ~ 59904ms
	rfm12_set_wakeup_timer(0b101001100111); // ~ 105472ms  DEFAULT VALUE!!!

	sei();

	bool needI2C = (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
		|| (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085)
		|| (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085);

	while (42)
	{
		// measure ADC dependant values
		adc_on(true);
		measure_battery_voltage();
		measure_brightness();
		adc_on(false);

		// measure other values, possibly I2C dependant
		if (needI2C)
		{
			i2c_enable();
		}

		measure_temperature();
		measure_humidity();
		measure_barometric_pressure();

		if (needI2C)
		{
			i2c_disable();
		}
		
		version_status_cycle++;

		// search for value to send with avgThr reached
		bool send = true;
		
		if (humidity.cnt >= humidity.avgThr)
		{
			prepare_humiditytemperature();
		}
		else if (barometric_pressure.cnt >= barometric_pressure.avgThr)
		{
			prepare_barometricpressuretemperature();
		}
		else if (temperature.cnt >= temperature.avgThr)
		{
			prepare_temperature();
		}
		else if (brightness.cnt >= brightness.avgThr)
		{
			prepare_brightness();
		}
		else if (battery_voltage.cnt >= battery_voltage.avgThr)
		{
			prepare_battery_voltage();
		}
		else if (version_status_cycle >= AVERAGE_COUNT_VERSION)
		{
			prepare_version();
		}
		else
		{
			send = false;
		}
		
		if (send)
		{
			pkg_header_set_senderid(device_id);
			pkg_header_set_packetcounter(packetcounter);
			pkg_header_calc_crc32();
			rfm12_send_bufx();
			rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

			switch_led(1);
			_delay_ms(200);
			switch_led(0);
			
			inc_packetcounter();
		}

		// go to sleep. Wakeup by RFM12 wakeup-interrupt
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
	}
}
