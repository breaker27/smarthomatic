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

#include "util.h"
#include "uart.h"

#include "e2p_access.h"

uint8_t bufx[65];

#define LED_PIN 7
#define LED_PORT PORTD
#define LED_DDR DDRD

// reference battery voltage (alkaline) for 100%, 90%,... 0% with end voltage 0,9V
//static short vbat_alkaline[] = {1600, 1400, 1320, 1280, 1240, 1210, 1180, 1160, 1100, 1030, 900};

// reference battery voltage (alkaline) for 100%, 90%,... 0% with end voltage 1,1V (= minimum for RFM12)
static short vbat_alkaline[] = {1600, 1405, 1333, 1293, 1260, 1232, 1205, 1183, 1170, 1145, 1100};

// PRECONDITION: min_in < max_in, min_out < max_out
uint16_t linear_interpolate(uint16_t in, uint16_t min_in, uint16_t max_in, uint16_t min_out, uint16_t max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

uint32_t linear_interpolate32(uint32_t in, uint32_t min_in, uint32_t max_in, uint32_t min_out, uint32_t max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

float linear_interpolate_f(float in, float min_in, float max_in, float min_out, float max_out)
{
	if (in >= max_in)
		return max_out;
	if (in <= min_in)
		return min_out;
	// interpolate
	return min_out + (in - min_in) * (max_out - min_out) / (max_in - min_in);
}

// return the remaining battery capacity according to the battery voltage
int bat_percentage(int vbat)
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
		int i = 0;
		
		while (vbat < vbat_alkaline[i])
		{
			i++;
		}
		
		return linear_interpolate(vbat, vbat_alkaline[i], vbat_alkaline[i - 1], 100 - i * 10, 100 - (i - 1) * 10);
	}
}

void adc_init(void)
{
	//{{WIZARD_MAP(ADC)
	// ADC Clock: 62.500kHz
	// ADC Voltage Reference: Internal 1.1V, External capacitor
	// ADC Noise Canceler Enabled
	ADCSRB |= 0x0;
	ADMUX = 0xc0;
	ADCSRA = 0x8e;
	//}}WIZARD_MAP(ADC)
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

unsigned int read_adc(unsigned char adc_input)
{
	// Set ADC input
	ADMUX &= 0xE0;
	ADMUX |= adc_input;
	// MCU sleep
	set_sleep_mode(SLEEP_MODE_ADC);
	sleep_mode();
	return adc_data;
}

/*
Grundlagen zu diesen Funktionen wurden der Webseite:
http://www.cs.waikato.ac.nz/~312/crc.txt
entnommen (A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS)

Algorithmus entsprechend CRC32 fuer Ethernet

Startwert FFFFFFFF, LSB zuerst
Im Ergebnis kommt MSB zuerst und alle Bits sind invertiert

Das Ergebnis wurde geprueft mit dem CRC-Calculator:
http://www.zorc.breitbandkatze.de/crc.html
(Einstellung Button CRC-32 waehlen, Daten eingeben und calculate druecken)

Autor: K.Moraw, www.helitron.de, Oktober 2009
*/

unsigned long reg32; 		// Schieberegister

unsigned long crc32_bytecalc(unsigned char byte)
{
	int i;
	unsigned long polynom = 0xEDB88320;		// Generatorpolynom

    for (i = 0; i < 8; ++i)
	{
        if ((reg32&1) != (byte&1))
             reg32 = (reg32>>1)^polynom; 
        else 
             reg32 >>= 1;
		byte >>= 1;
	}
	return reg32 ^ 0xffffffff;	 		// inverses Ergebnis, MSB zuerst
}

unsigned long crc32(unsigned char *data, int len)
{
	int i;
	reg32 = 0xffffffff;

	for (i = 0; i < len; i++)
	{
		crc32_bytecalc(data[i]);		// Berechne fuer jeweils 8 Bit der Nachricht
	}
	
	return reg32 ^ 0xffffffff;
}

// Return a 32 bit value stored in 4 bytes in the buffer at the given offset.
uint32_t getBuf32(uint8_t offset)
{
	return ((uint32_t)bufx[offset] << 24) | ((uint32_t)bufx[offset + 1] << 16) | ((uint32_t)bufx[offset + 2] << 8) | ((uint32_t)bufx[offset + 3] << 0);
}

// Return a 16 bit value stored in 2 bytes in the buffer at the given offset.
uint32_t getBuf16(uint8_t offset)
{
	return ((uint32_t)bufx[offset] << 8) | ((uint32_t)bufx[offset + 1] << 0);
}

// write a 32 bit value to the buffer
void setBuf32(uint8_t offset, uint32_t val)
{ 
	bufx[0 + offset] = (val >> 24) & 0xff;
	bufx[1 + offset] = (val >> 16) & 0xff;
	bufx[2 + offset] = (val >>  8) & 0xff;
	bufx[3 + offset] = (val >>  0) & 0xff;
}

// write a 16 bit value to the buffer
void setBuf16(uint8_t offset, uint16_t val)
{
	bufx[0 + offset]  = (val >> 8) & 0xff;
	bufx[1 + offset]  = (val >> 0) & 0xff;
}

void util_init(void)
{
	sbi(LED_DDR, LED_PIN);
}

void switch_led(uint8_t b_on)
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
	if (expectedDeviceType != eeprom_read_UIntValue8(EEPROM_DEVICETYPE_BYTE, EEPROM_DEVICETYPE_BIT,
		EEPROM_DEVICETYPE_LENGTH_BITS, 0, (1 << EEPROM_DEVICETYPE_LENGTH_BITS) - 1))
	{
		signal_error_state();
	}
}

// print an info over UART about the OSCCAL adjustment that was made
void osccal_info(void)
{
	uint8_t mode = eeprom_read_UIntValue8(EEPROM_OSCCALMODE_BYTE, EEPROM_OSCCALMODE_BIT,
		EEPROM_OSCCALMODE_LENGTH_BITS, EEPROM_OSCCALMODE_MINVAL, EEPROM_OSCCALMODE_MAXVAL);
	
	if ((mode > 0) && (mode < 255))
	{
		int16_t adjustment = (int16_t)mode - 128;
		UART_PUTF("The CPU speed was adjusted by +%d/1000 as set in OSCCAL_MODE byte.\r\n", adjustment);
	}
}

// Initialize the OSCCAL register, used to adjust the internal clock.
// Behaviour depends on OSCCAL_MODE EEPROM value:
// - 00: don't use OSCCAL calibration (e.g. external crystal oszillator is used)
// - FF: OSCCAL measure mode LED: The LED blinks every 60s, so the user can measure the original speed.
// - 01..FE: The speed is adjusted. If the value is X, the speed is adjusted by (X - 128) promille.
//           Ex: Setting the value to 138 adjusts the speed by (X - 128) promille = +1%.
void osccal_init(void)
{
	uint8_t mode = eeprom_read_UIntValue8(EEPROM_OSCCALMODE_BYTE, EEPROM_OSCCALMODE_BIT,
		EEPROM_OSCCALMODE_LENGTH_BITS, EEPROM_OSCCALMODE_MINVAL, EEPROM_OSCCALMODE_MAXVAL);
	
	if (mode == 255)
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
	else if (mode > 0)
	{
		float speedup = ((float)mode - 128) / 1000;
		OSCCAL = (uint16_t)((float)OSCCAL * (1 + speedup));
	}
}
