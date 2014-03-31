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
#include "../src_common/msggrp_envsensor.h"

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
#define SEND_BATT_STATUS_CYCLE 30 // send battery status x times less than temp status
#define SEND_VERSION_STATUS_CYCLE 200 // send version status x times less than temp status (~once per day)

uint8_t temperature_sensor_type = 0;
uint8_t brightness_sensor_type = 0;
uint8_t barometric_sensor_type = 0;
uint8_t batt_status_cycle = SEND_BATT_STATUS_CYCLE - 1; // send promptly after startup
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup

int main(void)
{
	uint16_t vbat = 0;
	uint16_t vlight = 0;
	int32_t temp = 0;
	uint16_t hum = 0;
	uint32_t baro = 0;
	uint8_t device_id = 0;
	uint8_t avg = 0;
	uint8_t bat_p_val = 0;
	uint16_t vempty = 1100; // 1.1V * 2 cells = 2.2V = min. voltage for RFM2

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_ENVSENSOR);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device specific config
	temperature_sensor_type = e2p_envsensor_get_temperaturesensortype();
	brightness_sensor_type = e2p_envsensor_get_brightnesssensortype();
	barometric_sensor_type = e2p_envsensor_get_barometricsensortype();

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
	UART_PUTF ("Brightness sensor type: %u\r\n", brightness_sensor_type);
	UART_PUTF ("Barometric Sensor Type: %u\r\n", barometric_sensor_type);
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

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
	
	rfm12_set_wakeup_timer(0b11100110000);   // ~ 6s
	//rfm12_set_wakeup_timer(0b11111000000);   // ~ 24576ms
	//rfm12_set_wakeup_timer(0b0100101110101); // ~ 59904ms
	//rfm12_set_wakeup_timer(0b101001100111); // ~ 105472ms  DEFAULT VALUE!!!

	sei();

	while (42)
	{
		// Measure voltage + brightness using ADCs
		adc_on(true);

		vbat += (int)((long)read_adc(0) * 34375 / 10000 / 2); // 1.1 * 480 Ohm / 150 Ohm / 1,024

		if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
		{
			vlight += read_adc(1);
		}

		adc_on(false);

		// Measure temperature (+ humidity in case of SHT15)
		if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
		{
			sht11_start_measure();
			_delay_ms(500);
			while (!sht11_measure_finish());
		
			temp += sht11_get_tmp();
			hum += sht11_get_hum();
		}
		else if (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
		{
			i2c_enable();
			lm75_wakeup();
			_delay_ms(lm75_get_meas_time_ms());
			temp += lm75_get_tmp();
			lm75_shutdown();
			i2c_disable();
		}

		if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
		{
			i2c_enable();
			baro += bmp085_meas_pressure();
			temp += bmp085_meas_temp();
			i2c_disable();
		}
			i2c_disable();
		}

		avg++;
		
		if (avg >= AVERAGE_COUNT)
		{
			vbat /= AVERAGE_COUNT;
			vlight /= AVERAGE_COUNT;
			temp /= AVERAGE_COUNT;
			hum /= AVERAGE_COUNT * 10;
			baro /= AVERAGE_COUNT;

			inc_packetcounter();

			// Set packet content
			pkg_header_init_envsensor_temphumbristatus_status();
			pkg_header_set_senderid(device_id);
			pkg_header_set_packetcounter(packetcounter);
			msg_envsensor_temphumbristatus_set_temperature(temp);
			msg_envsensor_temphumbristatus_set_humidity(hum);

			//TODO: find a place in bufx for baro data
			//setBuf32(11, baro);

			if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
			{
				msg_envsensor_temphumbristatus_set_brightness(100 - (int)((long)vlight * 100 / 1024));
			}

			pkg_header_calc_crc32();
			
			bat_p_val = bat_percentage(vbat, vempty);
			
			UART_PUTF("Battery: %u%%, Temperature: ", bat_p_val);
			print_signed(temp);
			UART_PUTF2(" deg.C, Humidity: %u.%u", hum / 10, hum % 10);
			UART_PUTS("%\r\n");
			UART_PUTF("Barometric: %ld pascal\r\n", baro);

			rfm12_send_bufx();
			rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

			switch_led(1);
			_delay_ms(200);
			switch_led(0);

			vbat = temp = baro = hum = vlight = avg = 0;
			batt_status_cycle++;
			version_status_cycle++;
		}
		else
		{
			if (batt_status_cycle >= SEND_BATT_STATUS_CYCLE)
			{
				batt_status_cycle = 0;
				inc_packetcounter();
				
				// Set packet content
				pkg_header_init_generic_batterystatus_status();
				pkg_header_set_senderid(device_id);
				pkg_header_set_packetcounter(packetcounter);
				msg_generic_batterystatus_set_percentage(bat_p_val);
				pkg_header_calc_crc32();
				
				UART_PUTF("Battery: %u%%\r\n", bat_p_val);

				rfm12_send_bufx();
				rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

				switch_led(1);
				_delay_ms(200);
				switch_led(0);
			}
			else if (version_status_cycle >= SEND_VERSION_STATUS_CYCLE)
			{
				version_status_cycle = 0;
				inc_packetcounter();
				
				// Set packet content
				pkg_header_init_generic_version_status();
				pkg_header_set_senderid(device_id);
				pkg_header_set_packetcounter(packetcounter);
				msg_generic_version_set_major(VERSION_MAJOR);
				msg_generic_version_set_minor(VERSION_MINOR);
				msg_generic_version_set_patch(VERSION_PATCH);
				msg_generic_version_set_hash(VERSION_HASH);
				pkg_header_calc_crc32();
				
				UART_PUTF4("Version: v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);

				rfm12_send_bufx();
				rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

				switch_led(1);
				_delay_ms(200);
				switch_led(0);
			}
		}
		
		// go to sleep. Wakeup by RFM12 wakeup-interrupt
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
	}
}
