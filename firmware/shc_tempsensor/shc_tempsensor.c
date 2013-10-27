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

// switch on debugging by UART
//#define UART_DEBUG

#include "sht11.h"

#include "aes256.h"
#include "util.h"

// check some assumptions at precompile time about flash layout
#if (EEPROM_AESKEY_BIT != 0)
	#error AES key does not start at a byte border. Not supported (maybe fix E2P layout?).
#endif

#define AVERAGE_COUNT 4 // Average over how many values before sending over RFM12?

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

uint32_t packetcounter = 0;
uint8_t temperature_sensor_type = 0;
uint8_t brightness_sensor_type = 0;

void printbytearray(uint8_t * b, uint8_t len)
{
	uint8_t i;
	
	for (i = 0; i < len; i++)
	{
		UART_PUTF("%02x ", b[i]);
	}
	
	UART_PUTS ("\r\n");
}

void rfm12_sendbuf(void)
{
	#ifdef UART_DEBUG
	UART_PUTS("Before encryption: ");
	printbytearray(bufx, 16);
	#endif

	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, 16);

	#ifdef UART_DEBUG
	UART_PUTS("After encryption:  ");
	printbytearray(bufx, aes_byte_count);
	#endif

	rfm12_tx(aes_byte_count, 0, (uint8_t *) bufx);
}

int main ( void )
{
	uint16_t vbat = 0;
	uint16_t vlight = 0;
	int16_t temp = 0;
	uint16_t hum = 0;
	uint8_t device_id = 0;
	uint8_t avg = 0;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_TEMPERATURE_SENSOR);

	// read packetcounter, increase by cycle and write back
	packetcounter = eeprom_read_UIntValue32(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT,
		EEPROM_PACKETCOUNTER_LENGTH_BITS, EEPROM_PACKETCOUNTER_MINVAL, EEPROM_PACKETCOUNTER_MAXVAL) + PACKET_COUNTER_WRITE_CYCLE;

	eeprom_write_UIntValue(EEPROM_PACKETCOUNTER_BYTE, EEPROM_PACKETCOUNTER_BIT, EEPROM_PACKETCOUNTER_LENGTH_BITS, packetcounter);

	// read device specific config
	temperature_sensor_type = eeprom_read_UIntValue8(EEPROM_TEMPERATURESENSORTYPE_BYTE,
		EEPROM_TEMPERATURESENSORTYPE_BIT, EEPROM_TEMPERATURESENSORTYPE_LENGTH_BITS, 0, 255);

	brightness_sensor_type = eeprom_read_UIntValue8(EEPROM_BRIGHTNESSSENSORTYPE_BYTE,
		EEPROM_BRIGHTNESSSENSORTYPE_BIT, EEPROM_BRIGHTNESSSENSORTYPE_LENGTH_BITS, 0, 255);

	// read device id and write to send buffer
	device_id = eeprom_read_UIntValue16(EEPROM_DEVICEID_BYTE, EEPROM_DEVICEID_BIT,
		EEPROM_DEVICEID_LENGTH_BITS, EEPROM_DEVICEID_MINVAL, EEPROM_DEVICEID_MAXVAL);
	
	osccal_init();
	
#ifdef UART_DEBUG
	uart_init(false);
	UART_PUTS ("\r\n");
	UART_PUTS ("smarthomatic Tempsensor V1.0 (c) 2013 Uwe Freese, www.smarthomatic.org\r\n");
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %u\r\n", packetcounter);
	UART_PUTF ("Temperature Sensor Type: %u\r\n", temperature_sensor_type);
	UART_PUTF ("Brightness Sensor Type: %u\r\n", brightness_sensor_type);
#endif
	
	// init AES key
	eeprom_read_block(aes_key, (uint8_t *)EEPROM_AESKEY_BYTE, 32);

	adc_init();

	if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
	{
	  sht11_init();
	}

	rfm12_init();
	//rfm12_set_wakeup_timer(0b11100110000);   // ~ 6s
	//rfm12_set_wakeup_timer(0b11111000000);   // ~ 24576ms
	//rfm12_set_wakeup_timer(0b0100101110101); // ~ 59904ms
	rfm12_set_wakeup_timer(0b101001100111); // ~ 105472ms  CORRECT VALUE!!!

	led_blink(100, 150, 20);

	sei();

	while (42)
	{
		// Measure using ADCs
		adc_on(true);

		vbat += (int)((long)read_adc(0) * 34375 / 10000 / 2); // 1.1 * 480 Ohm / 150 Ohm / 1,024

		if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
		{
			vlight += read_adc(1);
		}

		adc_on(false);

		// Measure SHT15
		if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
		{
			sht11_start_measure();
			_delay_ms(500);
			while (!sht11_measure_finish());
		
			temp += sht11_get_tmp();
			hum += sht11_get_hum();
		}

		avg++;
		
		if (avg >= AVERAGE_COUNT)
		{
			vbat /= AVERAGE_COUNT;
			vlight /= AVERAGE_COUNT;
			temp /= AVERAGE_COUNT;
			hum /= AVERAGE_COUNT;

			// set device ID
			bufx[0] = device_id;
			
			// update packet counter
			packetcounter++;
			
			if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
			{
				eeprom_write_dword((uint32_t*)0, packetcounter);
			}

			setBuf32(1, packetcounter);

			// set command ID 10 (Temperature Sensor Status)
			bufx[5] = 10;

			// update battery percentage
			bufx[6] = bat_percentage(vbat);

			// update temperature and humidity
			setBuf16(7, (uint16_t)temp);
			setBuf16(9, hum);

			// update brightness
			if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
			{
				bufx[11] = 100 - (int)((long)vlight * 100 / 1024);
			}
			uint32_t crc = crc32(bufx, 12);
#ifdef UART_DEBUG
			UART_PUTF("CRC32 is %lx (added as last 4 bytes)\r\n", crc);
#endif
			setBuf32(12, crc);

#ifdef UART_DEBUG
			UART_PUTF3("Battery: %u%%, Temperature: %d deg.C, Humidity: %d%%\r\n", bat_percentage(vbat), temp / 100.0, hum / 100.0);
#endif

			rfm12_sendbuf();
			
			rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

			switch_led(1);
			_delay_ms(200);
			switch_led(0);

			vbat = temp = hum = vlight = avg = 0;
		}
		
		// go to sleep. Wakeup by RFM12 wakeup-interrupt
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
	}
}
