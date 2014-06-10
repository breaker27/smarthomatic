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
#include "uart.h"

#include "../src_common/msggrp_generic.h"
#include "../src_common/msggrp_gpio.h"
#include "../src_common/msggrp_weather.h"
#include "../src_common/msggrp_environment.h"

#include "../src_common/e2p_hardware.h"
#include "../src_common/e2p_generic.h"
#include "../src_common/e2p_envsensor.h"

#include "sht11.h"
#include "i2c.h"
#include "lm75.h"
#include "bmp085.h"
#include "srf02.h"

#include "aes256.h"
#include "util.h"
#include "version.h"

#define AVERAGE_COUNT 4 // Average over how many values before sending over RFM12?
#define AVERAGE_COUNT_BATT 120
#define AVERAGE_COUNT_VERSION 800

#define SRF02_POWER_PORT PORTD
#define SRF02_POWER_DDR DDRD
#define SRF02_POWER_PIN 5

uint8_t temperature_sensor_type = 0;
uint8_t humidity_sensor_type = 0;
uint8_t barometric_sensor_type = 0;
uint8_t brightness_sensor_type = 0;
uint8_t distance_sensor_type = 0;
uint16_t version_status_cycle = AVERAGE_COUNT_VERSION - 1; // send promptly after startup
uint16_t vempty = 1100; // 1.1V * 2 cells = 2.2V = min. voltage for RFM2
bool di_change = false;
bool pin_wakeup = false; // remember if wakeup was done by pin change (or by RFM12B)

struct measurement_t
{
	int32_t val; // stores the accumulated value
	uint8_t cnt; // amount of single measurements
	uint8_t avgThr; // amount of measurements that are averaged into one value to send 
} temperature, humidity, barometric_pressure, distance, battery_voltage, brightness;

struct portpin_t
{
	uint8_t port;
	uint8_t pin;
	uint8_t mode;
	bool pull_up;
	struct measurement_t meas;
};

#define DI_UNUSED 255

struct portpin_t di[8];

// ---------- helper functions ----------

ISR (PCINT0_vect)
{
	pin_wakeup = true;
}

ISR (PCINT1_vect)
{
	pin_wakeup = true;
}

ISR (PCINT2_vect)
{
	pin_wakeup = true;
}

// enable the corresponding Pin Change Interrupt and mask the pin at the correct Pin Change Mask register
void enablePCI(uint8_t port_nr, uint8_t pin)
{
	if (port_nr == 2)
	{
		sbi(PCICR, PCIE2);
		sbi(PCMSK2, pin);
	}
	else if (port_nr == 1)
	{
		sbi(PCICR, PCIE1);
		sbi(PCMSK1, pin);
	}
	else
	{
		sbi(PCICR, PCIE0);
		sbi(PCMSK0, pin);
	}
}

uint8_t getPinStatus(uint8_t port_nr, uint8_t pin)
{
	uint8_t val;
	
	if (port_nr == 2)
		val = PIND;
	else if (port_nr == 1)
		val = PINC;
	else
		val = PINB;
		
	return (val >> pin) & 1;
}

void setPullUp(uint8_t port_nr, uint8_t pin)
{
	if (port_nr == 2)
		sbi(PIND, pin);
	else if (port_nr == 1)
		sbi(PINC, pin);
	else
		sbi(PINB, pin);
}

void clearPullUp(uint8_t port_nr, uint8_t pin)
{
	if (port_nr == 2)
		cbi(PIND, pin);
	else if (port_nr == 1)
		cbi(PINC, pin);
	else
		cbi(PINB, pin);
}

void init_di_sensor(void)
{
	uint8_t i;
	
	for (i = 0; i < 8; i++)
	{
		uint8_t pin = e2p_envsensor_get_digitalinputpins(i);
		
		if (pin == 0) // not used
		{
			di[i].port = DI_UNUSED;
			di[i].pull_up = false;
		}
		else
		{
			di[i].pull_up = e2p_envsensor_get_digitalinputpullupresistor(i);
			uint8_t mode = e2p_envsensor_get_digitalinputmode(i);

			di[i].port = (pin - 1) / 8;
			di[i].pin = (pin - 1) % 8;
			di[i].mode = mode;
			di[i].meas.cnt = 0;
			di[i].meas.val = 0;
			
			if (di[i].mode == DIGITALINPUTMODE_ONCHANGE)
			{
				enablePCI(di[i].port, di[i].pin); // enable Pin Change Interrupt
				
				if (di[i].pull_up)
				{
					setPullUp(di[i].port, di[i].pin); // when using PCI, pullups should be active
				}
			}

			// Send every 7 min. in cycle mode. Send immediately in "OnChange" mode and after 28 min.
			di[i].meas.avgThr = mode == DIGITALINPUTMODE_ONCHANGE ? AVERAGE_COUNT * 4 : AVERAGE_COUNT;

			UART_PUTF3("Using port %u pin %u as digital input pin %u ", di[i].port, di[i].pin, i);
			UART_PUTF2("in mode %u with pull-up %s\r\n", mode, di[i].pull_up ? "ON" : "OFF");
			
			// remember to send out status after power up
			di_change = true;
		}
	}
}

// ---------- functions to measure values from sensors ----------

void measure_digital_input(void)
{
	uint8_t i;
	bool wait_pullups = false;
	
	for (i = 0; i < 8; i++)
	{
		if ((di[i].pull_up) && (di[i].mode != DIGITALINPUTMODE_ONCHANGE))
		{
			setPullUp(di[i].port, di[i].pin);
			wait_pullups = true;
		}
	}
	
	// wait a little bit to let the voltage level settle down in case pullups were just switched on
	if (wait_pullups)
	{
		_delay_ms(10);
	}
	
	for (i = 0; i < 8; i++)
	{
		if (di[i].port != DI_UNUSED)
		{
			uint8_t stat = getPinStatus(di[i].port, di[i].pin);
			
			di[i].meas.cnt++;
			
			// if status changed or avgThr is reached, it is time to send
			if (((di[i].mode == DIGITALINPUTMODE_ONCHANGE) && (di[i].meas.val != stat))
				|| (di[i].meas.cnt >= di[i].meas.avgThr))
			{
				//UART_PUTS("Status change or vrgThr reached -> send\r\n");
				di_change = true;
			}
			
			di[i].meas.val = stat;
		}
	}
	
	for (i = 0; i < 8; i++)
	{
		if ((di[i].pull_up) && (di[i].mode != DIGITALINPUTMODE_ONCHANGE))
		{
			clearPullUp(di[i].port, di[i].pin);
		}
	}
}

void sht11_measure_loop(void)
{
	sht11_start_measure();
	_delay_ms(200);
	
	uint8_t i = 0;
	
	while (i < 100) // Abort at 1200ms. Measurement takes typically 420ms.
	{
		i++;
		_delay_ms(10);
		
		if (sht11_measure_finish())
		{
			//UART_PUTF("SHT15 measurement took %dms\r\n", 200 + i * 10);
			return;
		}
	}
	
	UART_PUTS("SHT15 measurement error.\r\n");
}

void measure_temperature_i2c(void)
{
	if (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
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

void measure_temperature_other(void)
{
	if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
	{
		sht11_measure_loop();
		temperature.val += sht11_get_tmp();
		temperature.cnt++;
	}
}

void measure_humidity_other(void)
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

void measure_barometric_pressure_i2c(void)
{
	if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
	{
		barometric_pressure.val += bmp085_meas_pressure();
		barometric_pressure.cnt++;
	}
}

void measure_distance_i2c(void)
{
	if (distance_sensor_type == DISTANCESENSORTYPE_SRF02)
	{
		distance.cnt++;
		
		// distance is measured only once (no averaging)
		if (distance.cnt == distance.avgThr)
		{
			distance.val = srf02_get_distance();
			//UART_PUTF("Dist = %d cm\r\n", distance.val);
		}
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

void prepare_digitalpin(void)
{
	pkg_header_init_gpio_digitalpin_status();

	UART_PUTS("Send GPIO: ");
	
	uint8_t i;
	
	for (i = 0; i < 8; i++)
	{
		
		if (di[i].port != DI_UNUSED)
		{
			UART_PUTF("%u", di[i].meas.val);
			di[i].meas.cnt = 0;
			
			msg_gpio_digitalpin_set_on(i, di[i].meas.val != 0);
		}
		else
		{
			UART_PUTS("-");
		}
	}
	
	UART_PUTS("\r\n");

	di_change = false;
}

void prepare_humiditytemperature(void)
{
	humidity.val = humidity.val / humidity.cnt; // in 100 * % rel.
	temperature.val /= temperature.cnt;

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
	barometric_pressure.val /= barometric_pressure.cnt;
	temperature.val /= temperature.cnt;

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
	msg_weather_temperature_set_temperature(temperature.val / temperature.cnt);
	
	UART_PUTS("Send temperature: ");
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");
	
	temperature.val = temperature.cnt = 0;
}

void prepare_distance(void)
{
	pkg_header_init_environment_distance_status();
	msg_environment_distance_set_distance(distance.val);
	
	UART_PUTF("Send distance: %d\r\n", distance.val);
	
	distance.val = distance.cnt = 0;
}
void prepare_brightness(void)
{
	brightness.val /= brightness.cnt;
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
	distance_sensor_type = e2p_envsensor_get_distancesensortype();

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
	UART_PUTF ("Humidity sensor type: %u\r\n", humidity_sensor_type);
	UART_PUTF ("Barometric sensor type: %u\r\n", barometric_sensor_type);
	UART_PUTF ("Brightness sensor type: %u\r\n", brightness_sensor_type);
	UART_PUTF ("Distance sensor type: %u\r\n", distance_sensor_type);
	
	init_di_sensor();
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	// Different average counts per measurement would be possible.
	// For now, use the same fixed value for all measurements.
	temperature.avgThr = humidity.avgThr = barometric_pressure.avgThr = brightness.avgThr = distance.avgThr = AVERAGE_COUNT;
	battery_voltage.avgThr = AVERAGE_COUNT_BATT;

	adc_init();

	// set DIDR for ADC channels, switch off digital input buffers to reduce ADC noise and to save power
	sbi(DIDR0, 0);
	sbi(DIDR0, 1);

	if ((temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
		|| (humidity_sensor_type == HUMIDITYSENSORTYPE_SHT15))
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

	if (distance_sensor_type == DISTANCESENSORTYPE_SRF02)
	{
		sbi(SRF02_POWER_DDR, SRF02_POWER_PIN);
	}

	led_blink(500, 500, 3);
	
	rfm12_init();
	
	//rfm12_set_wakeup_timer(0b11100110000);   // ~ 6s
	//rfm12_set_wakeup_timer(0b11111000000);   // ~ 24576ms
	//rfm12_set_wakeup_timer(0b0100101110101); // ~ 59904ms
	rfm12_set_wakeup_timer(0b101001100111); // ~ 105472ms  DEFAULT VALUE!!!

	sei();

	// If a SRF02 is connected, it is assumed that it is powered by a step up voltage converter,
	// which produces 5V out of the ~3V battery power. It has to be switched on to activate
	// the SRF02 and also make it possible to communicate to other I2C devices.

	bool srf02_connected = distance_sensor_type == DISTANCESENSORTYPE_SRF02;
	bool measure_other_i2c = (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
		|| (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085)
		|| (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085);

	while (42)
	{
		if (pin_wakeup)
		{
			measure_digital_input();
		}
		else // wakeup by RFM12B -> measure everything
		{
			bool measure_srf02 = srf02_connected && (distance.cnt + 1 == distance.avgThr); // SRF02 only measures every avgThr cycles!
			bool measure_i2c = measure_srf02 || measure_other_i2c;
			bool needs_power = measure_srf02 || (measure_other_i2c && srf02_connected);

			// measure ADC dependant values
			adc_on(true);
			measure_battery_voltage();
			measure_brightness();
			adc_on(false);
			measure_digital_input();

			// measure other values from I2C devices
			if (measure_i2c)
			{
				if (needs_power)
				{
					sbi(SRF02_POWER_PORT, SRF02_POWER_PIN);
					_delay_ms(1000); // ~500ms are needed to make the output voltage of the regulator stable
				}

				i2c_enable();
				measure_temperature_i2c();
				measure_barometric_pressure_i2c();
				measure_distance_i2c();
				i2c_disable();

				if (needs_power)
				{
					cbi(SRF02_POWER_PORT, SRF02_POWER_PIN);
				}
			}
			
			if (srf02_connected && !measure_srf02)
			{
				distance.cnt++; // increase distance counter, because measure_distance_i2c() was not called
			}
			
			// measure other values, non-I2C devices
			measure_temperature_other();
			measure_humidity_other();
			
			version_status_cycle++;
		}

		// search for value to send with avgThr reached
		bool send = true;
		
		if (di_change)
		{
			prepare_digitalpin();
		}
		else if (humidity.cnt >= humidity.avgThr)
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
		else if (distance.cnt >= distance.avgThr)
		{
			prepare_distance();
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

		// Go to sleep. Wakeup by RFM12 wakeup-interrupt or pin change (if configured).
		pin_wakeup = false;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
	}
}
