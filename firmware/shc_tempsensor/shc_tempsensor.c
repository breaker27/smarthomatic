/*
* This file is part of smarthomatic, http://www.smarthomatic.com.
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

// switch on voltage check of battery on PC0 (ADC0)
#define VBAT_SENSOR

// switch on SHT15 temperature / humidity sensor on PC2 + PC3
#define TEMP_SENS

// switch on light sensor on PC1 (ADC1)
#define LIGHT_SENS

#ifdef TEMP_SENS
	#include "sht11.h"
#endif

#include "../src_common/aes256.h"
#include "util.h"

#define AVERAGE_COUNT 4 // Average over how many values before sending over RFM12?

#define EEPROM_POS_DEVICE_ID 8
#define EEPROM_POS_PACKET_COUNTER 9
#define EEPROM_POS_AES_KEY 32

// How often should the packetcounter_base be increased and written to EEPROM?
// This should be 2^32 (which is the maximum transmitted packet counter) /
// 100.000 (which is the maximum amount of possible EEPROM write cycles) or more.
// Therefore 100 is a good value.
#define PACKET_COUNTER_WRITE_CYCLE 100

uint32_t packetcounter = 0;

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

	// read packetcounter, increase by cycle and write back
	packetcounter = eeprom_read_dword((uint32_t*)EEPROM_POS_PACKET_COUNTER) + PACKET_COUNTER_WRITE_CYCLE;
	eeprom_write_dword((uint32_t*)0, packetcounter);

	// read device id and write to send buffer
	device_id = eeprom_read_byte((uint8_t*)EEPROM_POS_DEVICE_ID);	
	
	//osccal_init();
	
#ifdef UART_DEBUG
	uart_init();
	UART_PUTS ("\r\n");
	UART_PUTS ("smarthomatic Tempsensor V1.0 (c) 2013 Uwe Freese, www.smarthomatic.com\r\n");
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %u\r\n", packetcounter);
#endif
	
	// init AES key
	eeprom_read_block (aes_key, (uint8_t *)EEPROM_POS_AES_KEY, 32);

#ifdef VBAT_SENSOR
	adc_init();
#endif

#ifdef TEMP_SENS
	sht11_init();
#endif

	rfm12_init();
	//rfm12_set_wakeup_timer(0b11100110000);   // ~ 6s
	//rfm12_set_wakeup_timer(0b11111000000);   // ~ 24576ms
	//rfm12_set_wakeup_timer(0b0100101110101); // ~ 59904ms
	rfm12_set_wakeup_timer(0b101001100111); // ~ 105472ms  CORRECT VALUE!!!

	led_blink(100, 150, 20);

	sei();

	while (42)
	{
#if defined VBAT_SENSOR || defined LIGHT_SENS
		adc_on(true);
#endif

#ifdef VBAT_SENSOR
		vbat += (int)((long)read_adc(0) * 34375 / 10000 / 2); // 1.1 * 480 Ohm / 150 Ohm / 1,024
#endif

#ifdef LIGHT_SENS
		vlight += read_adc(1);
#endif

#if defined VBAT_SENSOR || defined LIGHT_SENS
		adc_on(false);
#endif

#ifdef TEMP_SENS
		sht11_start_measure();
		_delay_ms(500);
		while (!sht11_measure_finish());
		
		temp += sht11_get_tmp();
		hum += sht11_get_hum();
#endif

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
#ifdef TEMP_SENS
			setBuf16(7, (uint16_t)temp);
			setBuf16(9, hum);
#endif

			// update brightness
#ifdef LIGHT_SENS
			bufx[11] = 100 - (int)((long)vlight * 100 / 1024);
#endif
			uint32_t crc = crc32(bufx, 12);
			UART_PUTF("CRC32 is %lx (added as last 4 bytes)\r\n", crc);
			setBuf32(12, crc);

#ifdef UART_DEBUG
//			UART_PUTF3("Battery: %u%%, Temperature: %d deg.C, Humidity: %d%%\r\n", bat_percentage(vbat), temp / 100.0, hum / 100.0);
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
