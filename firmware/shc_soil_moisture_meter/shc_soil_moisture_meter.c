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
#include <avr/sleep.h>

#include "rfm12.h"
#include "../src_common/uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_weather.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_soilmoisturemeter.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

// Don't change this, because other switch count like 8 needs other status message.
// If support implemented, use EEPROM_SUPPORTEDSWITCHES_* E2P addresses.
#define SWITCH_COUNT 1

#define TRIGGER_DDR DDRC
#define TRIGGER_PIN 2
#define TRIGGER_PORT PORTC

#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PINPORT PIND
#define BUTTON_PIN 3

#define BUTTON (!(BUTTON_PINPORT & (1 << BUTTON_PIN)))

#define SEND_STATUS_EVERY_SEC 1800 // how often should a status be sent?
#define SEND_BATTERY_STATUS_CYCLE 15 // send version status every x wake ups
#define SEND_VERSION_STATUS_CYCLE 50 // send version status every x wake ups

#define DENOISE_WINDOW_PERMILL 20

uint16_t device_id;
uint32_t station_packetcounter;
bool switch_state[SWITCH_COUNT];
uint16_t switch_timeout[SWITCH_COUNT];

uint16_t send_status_timeout = 5;
uint8_t version_status_cycle = SEND_VERSION_STATUS_CYCLE - 1; // send promptly after startup
uint8_t battery_status_cycle = SEND_BATTERY_STATUS_CYCLE - 1; // send promptly after startup

uint32_t dry_thr;              // configured by user
uint32_t counter_min = 100000; // min occurred value in current watering period
uint32_t counter_meas = 0;
bool init_mode = false;

uint16_t wupCnt = 0; // Amount of wake-up cycles counting from the last time a measurement was done.
uint16_t avgInt = 3; // The number of times a value is measured before an average is calculated and sent.
uint16_t avgIntInit = 3;

uint32_t reported_result = 0;
bool direction_up = true;

// TODO: Move to util
// calculate x^y
uint32_t power(uint32_t x, uint32_t y)
{
	uint32_t result = 1;

	while (y > 0)
	{
		result *= x;
		y--;
	}

	return result;
}

// Read wakeup timer value from e2p, config rfm12 and
// return the value (in seconds).
uint16_t init_wakeup(void)
{
	uint16_t interval = init_mode ? 
		e2p_soilmoisturemeter_get_wakeupintervalinit() :
		e2p_soilmoisturemeter_get_wakeupinterval();

	if (interval == 0) // misconficuration in E2P
	{
		interval = WAKEUPINTERVAL_1H;
	}
	
	rfm12_set_wakeup_timer(interval);
	
	// Calculate wake-up time in seconds according RFM12B datasheet and round the value to seconds.
	uint16_t sec = (uint16_t)(((interval & 0xff) * power(2, (interval >> 8) & 0b11111) + 500) / 1000);
	UART_PUTF("Wake-up interval: %us\r\n", sec);
	
	return sec;
}

void switch_schmitt_trigger(bool b_on)
{
	if (b_on)
	{
		sbi(TRIGGER_PORT, TRIGGER_PIN);
	}
	else
	{
		cbi(TRIGGER_PORT, TRIGGER_PIN);
	}
}

void send_version_status(void)
{
	UART_PUTF4("Sending Version: v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	// Set packet content
	inc_packetcounter();
	pkg_header_init_generic_version_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_version_set_major(VERSION_MAJOR);
	msg_generic_version_set_minor(VERSION_MINOR);
	msg_generic_version_set_patch(VERSION_PATCH);
	msg_generic_version_set_hash(VERSION_HASH);

	rfm12_send_bufx();
}

void send_battery_status(void)
{
	adc_on(true);
	_delay_ms(10);
	uint16_t percentage = bat_percentage(read_battery(), 1100); // 1.1V * 2 cells = 2.2V = min. voltage for RFM12B
	adc_on(false);

	UART_PUTF("Sending battery: %u%%\r\n", percentage);

	// Set packet content
	inc_packetcounter();
	pkg_header_init_generic_batterystatus_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_generic_batterystatus_set_percentage(percentage);

	rfm12_send_bufx();
}

// Send humidity
void send_humidity_status(uint16_t hum)
{	
	UART_PUTF2("Sending humidity: %u.%u%%\r\n", hum / 10, hum % 10);

	// Set packet content
	inc_packetcounter();
	pkg_header_init_weather_humidity_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_weather_humidity_set_humidity(hum);

	rfm12_send_bufx();
}

// Send humidity and raw value as temperature (only for debugging!)
void send_humidity_status_RAW_DBG(uint16_t hum, int16_t raw)
{	
	UART_PUTF2("Sending humidity: %u.%u%%\r\n", hum / 10, hum % 10);
	UART_PUTF ("Sending temperature: %d\r\n", raw);

	// Set packet content
	inc_packetcounter();
	pkg_header_init_weather_humiditytemperature_status();
	pkg_header_set_senderid(device_id);
	pkg_header_set_packetcounter(packetcounter);
	msg_weather_humiditytemperature_set_humidity(hum);
	msg_weather_humiditytemperature_set_temperature(raw);

	rfm12_send_bufx();
}

// Measure humidity, calculate relative value in permill and return it.
// Return true, if humidity was sent.
bool measure_humidity(void)
{
	bool res = false;
	
	switch_schmitt_trigger(true);
	_delay_ms(10);
	
	uint16_t result;

	// make PD5 an input and disable pull-ups
	DDRD &= ~(1 << 5);
	PORTD &= ~(1 << 5);

	// clear counter
	TCNT1 = 0x00;

	// configure counter and use external clock source, rising edge
	TCCR1A = 0x00;
	TCCR1B |= (1 << CS12) | (1 << CS11) | (1 << CS10);

	_delay_ms(100);

	//result = (TCNT1H << 8) | TCNT1L;
	result = TCNT1;

	TCCR1B = 0x00;  // turn counter off
	
	switch_schmitt_trigger(false);
	
	counter_meas += result;
	wupCnt++;
	
	UART_PUTF2("Measurement %u, Counter %u\r\n", wupCnt, result);
	
	if ((init_mode && (wupCnt == avgIntInit)) || (!init_mode && (wupCnt == avgInt)))
	{
		uint32_t avg = init_mode ? counter_meas / avgIntInit : counter_meas / avgInt;
		
		if (init_mode)
		{
			UART_PUTF("Init: Save avg %u as dry threshold.\r\n", avg);
			dry_thr = avg;
			counter_min = dry_thr - 1;
			init_mode = false;
			init_wakeup(); // to normal value
			e2p_soilmoisturemeter_set_drythreshold(dry_thr);
		}
		else
		{
			int32_t result;
		
			if (avg < counter_min)
			{
				counter_min = avg;
				UART_PUTF("New min: %lu, ", counter_min);
			}
		
			if (avg > dry_thr)
			{
				result = 0;
			}
			else
			{
				result = (dry_thr - avg) * 1000 / (dry_thr - counter_min);
			}
		
			UART_PUTF("Avg: %u, ", avg);
			UART_PUTF("Result: %lu permill\r\n", result);
			
			// Don't change reported value if it changes within a window of
			// some percent.
			if (reported_result == 0)
			{
				reported_result = result;
			}
			else
			{
				if (direction_up)
				{
					if (result > reported_result)
					{
						reported_result = result;
					}
					else if (result < reported_result - DENOISE_WINDOW_PERMILL)
					{
						reported_result = result;
						direction_up = false;
					}
				}
				else // direction down
				{
					if (result < reported_result)
					{
						reported_result = result;
					}
					else if (result > reported_result + DENOISE_WINDOW_PERMILL)
					{
						reported_result = result;
						direction_up = true;
					}
				}
			}
			
			send_humidity_status(reported_result);
			//send_humidity_status_RAW_DBG(reported_result, (int16_t) MIN((int32_t)avg, 30000)); // for debugging only
			res = true;
		}

		wupCnt = 0;
		counter_meas = 0;
	}
	
	_delay_ms(10);
	return res;
}

ISR (INT1_vect)
{
    /* interrupt code here */
}

int main(void)
{
	uint8_t loop = 0;
	uint16_t wakeup_sec;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();
	
	check_eeprom_compatibility(DEVICETYPE_SOILMOISTUREMETER);

	// init button input
	cbi(BUTTON_DDR, BUTTON_PIN);
	//sbi(BUTTON_PORT, BUTTON_PIN);
	
	
	// init power pin for 74HC14D
	sbi(TRIGGER_DDR, TRIGGER_PIN);

	// read packetcounter, increase by cycle and write back
	packetcounter = e2p_generic_get_packetcounter() + PACKET_COUNTER_WRITE_CYCLE;
	e2p_generic_set_packetcounter(packetcounter);

	// read device id
	device_id = e2p_generic_get_deviceid();

	dry_thr = e2p_soilmoisturemeter_get_drythreshold();
	if (dry_thr == 0) // set default value if never initialized
	{
		dry_thr = 40000;
	}

	counter_min = e2p_soilmoisturemeter_get_minval();
	if (counter_min == 0) // set default value if never initialized
	{
		counter_min = 30000;
	}

	avgIntInit = e2p_soilmoisturemeter_get_averagingintervalinit();
	avgInt = e2p_soilmoisturemeter_get_averaginginterval();

	osccal_init();

	uart_init();

	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic Soil Moisture Meter v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2014 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("DeviceID: %u\r\n", device_id);
	UART_PUTF ("PacketCounter: %lu\r\n", packetcounter);
	UART_PUTF ("AveragingInterval for initialization: %u\r\n", avgIntInit);
	UART_PUTF ("AveragingInterval for normal operation: %u\r\n", avgInt);
	UART_PUTF ("Dry threshold: %u\r\n", dry_thr);
	UART_PUTF ("Min value: %u\r\n", counter_min);

	adc_init();

	// set DIDR for ADC channels, switch off digital input buffers to reduce ADC noise and to save power
	DIDR0 = 62; // 63 for all channels!
	DIDR1 = 3;
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	led_blink(500, 500, 3);

	rfm12_init();
	wakeup_sec = init_wakeup();

	// init interrupt for button (falling edge)
	sbi(EICRA, ISC11);
	sbi(EIMSK, INT1);
	
	// set pull-up for BUTTON_DDR
	sbi(BUTTON_PINPORT, BUTTON_PIN);
	
	sei();

	while (42)
	{
		if (BUTTON)
		{
			led_blink(100, 0, 1);
			UART_PUTS("Button pressed!\r\n");
			
			uint8_t cnt = 0;
			
			while (BUTTON && (cnt < 250))
			{
				_delay_ms(10);
				cnt++;
			}
			
			if (cnt == 250)
			{
				UART_PUTS("Long press -> initiate measure mode!\r\n");
				
				while (BUTTON)
				{
					led_blink(100, 100, 1);
				}

				init_mode = true;
				wupCnt = 0;
				counter_meas = 0;
				init_wakeup(); // to usually shorter value
				
				UART_PUTS("Button released!\r\n");
				_delay_ms(10);
			}
		}
		else
		{
			// send status from time to time
			send_status_timeout--;
		
			if (!measure_humidity())
			{
				if (battery_status_cycle > 0)
					battery_status_cycle--;

				if (version_status_cycle > 0)
					version_status_cycle--;

				if (battery_status_cycle == 0)
				{
					battery_status_cycle = SEND_BATTERY_STATUS_CYCLE;
					send_battery_status();
					led_blink(200, 0, 1);
				}
				else if (version_status_cycle == 0)
				{
					version_status_cycle = SEND_VERSION_STATUS_CYCLE;
					send_version_status();
					led_blink(200, 0, 1);
				}
			}

			rfm12_tick();
			
			loop++;
		}
		
		power_down(true);
	}
	
	// never called
	// aes256_done(&aes_ctx);
}
