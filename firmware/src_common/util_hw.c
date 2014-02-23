/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
*
* Contributors:
*    CRC32: K.Moraw, www.helitron.de, 2009
*
* Development for smarthomatic by Uwe Freese started by adding a
* function to encrypt and decrypt a byte buffer directly and adding CBC mode.
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
#include <stdlib.h>
#include <avr/sleep.h>
#include <string.h>
#include <stdio.h>
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>
#include <avr/eeprom.h>

#include "util_generic.h"
#include "util_hw.h"
#include "uart.h"

#include "e2p_access.h"
#include "e2p_hardware.h"
#include "e2p_generic.h"
#include "packet_header.h"
#include "rfm12.h"
#include "aes256.h"

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

static uint16_t adc_data;

// reference battery voltage (alkaline) for 100%, 90%,... 0% with end voltage 0,9V
static short vbat_alkaline[] = {1600, 1400, 1320, 1280, 1240, 1210, 1180, 1160, 1100, 1030, 900};

// Return the remaining battery capacity according to the battery voltage,
// without considering the minimum voltage for the device.
static uint16_t _bat_percentage_raw(uint16_t vbat)
{
	if (vbat >= vbat_alkaline[0])
	{
		return 100;
	}
	else if (vbat <= vbat_alkaline[10])
	{
		return 0;
	}
	else // linear interpolation between the array values
	{
		uint8_t i = 0;
		
		while (vbat < vbat_alkaline[i])
		{
			i++;
		}
		
		return linear_interpolate16(vbat, vbat_alkaline[i], vbat_alkaline[i - 1], 100 - i * 10, 100 - (i - 1) * 10);
	}
}

// Return the remaining battery capacity according to the battery voltage,
// considering the minimum voltage for the device as well.
// Only this function should be called from the main file.
uint16_t bat_percentage(uint16_t vbat, uint16_t vempty)
{
	uint16_t percentage = _bat_percentage_raw(vbat);
	uint16_t min_percentage = _bat_percentage_raw(vempty);
	
	if (percentage < min_percentage)
	{
		return 0;
	}
	else
	{
		return (percentage - min_percentage) * 100 / (100 - min_percentage);
	}
}

void adc_init(void)
{
	// ADC Clock: 62.500kHz
	// ADC Voltage Reference: Internal 1.1V, External capacitor
	// ADC Noise Canceler Enabled
	ADCSRB |= 0x0;
	ADMUX = 0xc0;
	ADCSRA = 0x8e;
}

void adc_on(bool on)
{
    //ACSR = (1<<ACD);                      // disable A/D comparator
    if (on)
	{
		sbi(ADCSRA, ADEN);
	}
	else
	{
		cbi(ADCSRA, ADEN);
	}
}

// Read the ADC conversion result after ADC is finished
ISR(ADC_vect)
{
	adc_data = ADCW;
}

uint16_t read_adc(uint8_t adc_input)
{
	// Set ADC input
	ADMUX &= 0xE0;
	ADMUX |= adc_input;
	// MCU sleep
	set_sleep_mode(SLEEP_MODE_ADC);
	sleep_mode();
	return adc_data;
}


void util_init(void)
{
	sbi(LED_DDR, LED_PIN);
}

void switch_led(bool b_on)
{
	if (b_on)
	{
		sbi(LED_PORT, LED_PIN);
	}
	else
	{
		cbi(LED_PORT, LED_PIN);
	}
}

void led_blink(uint16_t on, uint16_t off, uint8_t times)
{
	uint8_t i;
	
	for (i = 0; i < times; i++)
	{
		sbi(LED_PORT, LED_PIN);
		_delay_ms(on);
		cbi(LED_PORT, LED_PIN);
		_delay_ms(off);
	}
}

// Signal a serious error state and do nothing further except LED blinking.
void signal_error_state(void)
{
	while (1)
	{
		led_blink(50, 200, 1);
	}
}

// Check if the EEPROM is compatible to the device by checking against the device type byte in EEPROM.
// If not, wait in endless loop and let LED blink.
void check_eeprom_compatibility(uint8_t expectedDeviceType)
{
	if (expectedDeviceType != e2p_hardware_get_devicetype())
	{
		signal_error_state();
	}
}

// print an info over UART about the OSCCAL adjustment that was made
void osccal_info(void)
{
#ifdef UART_DEBUG
	int8_t mode = e2p_hardware_get_osccalmode();
	
	if (mode != 0)
	{
		UART_PUTF("The CPU speed was adjusted by %+d/1000 as set in OSCCAL_MODE byte.\r\n", mode);
	}
#endif // UART_DEBUG
}

// Initialize the OSCCAL register, used to adjust the internal clock.
// Behaviour depends on OSCCAL_MODE EEPROM value:
// - 0: don't use OSCCAL calibration (e.g. external crystal oszillator is used)
// - -128: OSCCAL measure mode LED: The LED blinks every 60s, so the user can measure the original speed.
// - -127..127: The speed is adjusted.
//          Ex: Setting the value to 10 adjusts the speed by +1%.
void osccal_init(void)
{
	int8_t mode = e2p_hardware_get_osccalmode();
	
	if (mode == -128)
	{
		uint8_t i;
		
		while (1)
		{
			led_blink(120, 0, 1);
	
			for (i = 0; i < 12; i++)
			{
				_delay_ms(4990); // _delay_ms can handle only about ~6s
			}
		}
	}
	else if (mode != 0)
	{
		float speedup = (float)mode / 1000;
		OSCCAL = (uint16_t)((float)OSCCAL * (1 + speedup));
	}
}

void inc_packetcounter(void)
{
	packetcounter++;
	
	if (packetcounter % PACKET_COUNTER_WRITE_CYCLE == 0)
	{
		e2p_generic_set_packetcounter(packetcounter);
	}
}

void rfm12_send_bufx(void)
{
	UART_PUTS("Before encryption: ");
	print_bytearray(bufx, __PACKETSIZEBYTES);

	uint8_t aes_byte_count = aes256_encrypt_cbc(bufx, __PACKETSIZEBYTES);

	UART_PUTS("After encryption:  ");
	print_bytearray(bufx, aes_byte_count);

	rfm12_tx(aes_byte_count, 0, (uint8_t *) bufx);
}
