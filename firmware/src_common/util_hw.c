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

// Value has to be volatile, because otherwise the adc_measure function would not
// detect that the value was changed from the ADC interrupt.
volatile static uint16_t adc_data;

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

// Initialize ADV (called once after initial power on).
void adc_init(void)
{
	// ADIE: A/D Interrupt enable
	// ADPS0 / 1 / 2 Prescalers
	// The prescaler has to be set to result in a frequency of 50kHz to 200kHz.
	// Choose a prescaler so that frequency is as little above 50kHz as possible.
	// CPU clock below 1.6 MHz -> Prescaler 16 -> ADPS2
	// CPU clock below 3.2 MHz -> Prescaler 32 -> ADPS2 + ADPS0
	// CPU clock below 6.4 MHz -> Prescaler 64 -> ADPS2 + ADPS1
	// CPU clock above 6.4 MHz -> Prescaler 128 -> ADPS2 + ADPS1 + ADPS0
#if (F_CPU < 1600000)
	ADCSRA = (1 << ADIE) | (1 << ADPS2);
#elif (F_CPU < 3200000)
	ADCSRA = (1 << ADIE) | (1 << ADPS2) | (1 << ADPS0);
#elif (F_CPU < 6400000)
	ADCSRA = (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1);
#else
	ADCSRA = (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
#endif 

	// Voltage reference selection:
	// REFS1 = 1, REFS0 = 1 -> internal 1.1V reference (used for all ADC input pins)
	// REFS1 = 0, REFS0 = 1 -> AVcc (used to measure battery voltage)
	// -> In any case, REFS0 has to be 1.
	sbi(ADMUX, REFS0);
}

// Switch on ADC before taking measurements and switch it off afterwards,
// especially before going to power down mode.
void adc_on(bool on)
{
	if (on)
	{
		sbi(ADCSRA, ADEN);
	}
	else
	{
		cbi(ADCSRA, ADEN);
	}
}

// Read the ADC conversion result after ADC is finished.
ISR(ADC_vect)
{
	adc_data = ADCW;
}

// Make ADC conversion in ADC sleep mode.
// If device is woken up by another interrupt, retry until an ADC conversion
// without disturbance was possible.
void adc_measure(void)
{
	set_sleep_mode(SLEEP_MODE_ADC);
	adc_data = 0xffff;

	while (true)
	{
		// Go into ADC sleep mode. This starts an DC conversion automatically.
		// Device will wake up either by an ADC conversion complete interrupt
		// or by any other interrupt (!).
		sleep_enable();
		sei();
		sleep_cpu();
		
		// Disable interrupts immediately to make sure the ADC interrupt does
		// not happen right here after the device was woken up by another interrupt.
		cli();
		
		// Check if device was woken up by ADC interrupt and ADC value was
		// therefore stored in adc_data.
		if (adc_data != 0xffff)
		{
			break;
		}

		// If woken up by another interrupt, wait until conversion is complete
		// and start another conversion in the loop.
		while (ADCSRA & (1<<ADSC))
		{
			// NOOP
		}
	}

	sei();
}

// Read ADC data of the given channel.
// At channel 0..7 (ADC0..ADC7), internal reference voltage of 1.1V is used.
// Channel 14 is used to measure the internal reference voltage of 1.1V itself,
// with a temporary voltage reference switched to AVCC (= VCC = Battery voltage).
// This makes calculation of the battery voltage possible without external
// resistors.
uint16_t read_adc(uint8_t adc_input)
{
	// Set voltage reference.
	if (adc_input != 14)
	{
		sbi(ADMUX, REFS1);
	}
	else
	{
		cbi(ADMUX, REFS1);
	}
	
	// Set input channel.
	ADMUX = (ADMUX & 0b11110000) | adc_input;

	// Make two conversions, because the first value can be wrong
	// after changing voltage reference (also according datasheet).
	adc_measure();
	adc_measure();

	return adc_data;
}

// Calculate current battery voltage by using it as reference voltage and measuring
// the internal 1.1V reference voltage.
uint16_t read_battery(void)
{
	//return (int)((long)read_adc(0) * 34375 / 10000); // using external resistors
	return (uint16_t)((uint32_t)1100 * 1024 / read_adc(14));
}

void util_init(void)
{
	sbi(LED_DDR, LED_PIN);
}

void led_dbg(uint8_t ms)
{
	sbi(LED_PORT, LED_PIN);
	_delay_ms(ms);
	cbi(LED_PORT, LED_PIN);
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
void check_eeprom_compatibility(DeviceTypeEnum expectedDeviceType)
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

// Truncate trailing 0-bytes, round up to packet length of multiple of 16 bytes,
// set CRC, encode and send packet with RFM12.
void rfm12_send_bufx(void)
{
	while ((__PACKETSIZEBYTES > 0) && (bufx[__PACKETSIZEBYTES - 1] == 0))
	{
		__PACKETSIZEBYTES--;
	}
	
	__PACKETSIZEBYTES = ((__PACKETSIZEBYTES - 1) / 16 + 1) * 16;

	uint32_t crc = crc32(bufx + 4, __PACKETSIZEBYTES - 4);
	array_write_UIntValue(0, 32, crc, bufx);
	
	UART_PUTS("Before encryption: ");
	print_bytearray(bufx, __PACKETSIZEBYTES);

	uint8_t packet_len = aes256_encrypt_cbc(bufx, __PACKETSIZEBYTES);

	// Write to tx buffer and call rfm12_tick to send immediately.
	rfm12_tx(packet_len, 0, (uint8_t *) bufx);
	rfm12_tick();
	//led_dbg(2);

	// Print to UART after sending to not cause additional delay.
	UART_PUTS("After encryption:  ");
	print_bytearray(bufx, packet_len);
}

// Go to sleep. Wakeup by RFM12 wakeup-interrupt or pin change (if configured).
// Disable BOD according recommended procedure in sleep.h if selected.
void power_down(bool bod_disable)
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();

// ATMega329 (thermostat) does not support BOD disable.
#if ! defined (__AVR_ATmega329__)
	if (bod_disable)
	{
		sleep_bod_disable();
	}
#endif

	sei();
	sleep_cpu();
	sleep_disable();
	sei();
}
