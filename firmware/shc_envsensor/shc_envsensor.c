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

#define SRF02_POWER_PORT PORTD
#define SRF02_POWER_DDR DDRD
#define SRF02_POWER_PIN 5

uint8_t temperature_sensor_type = 0;
uint8_t humidity_sensor_type = 0;
uint8_t barometric_sensor_type = 0;
uint8_t brightness_sensor_type = 0;
uint8_t distance_sensor_type = 0;
uint32_t version_measInt;
uint32_t version_cnt;
uint16_t vempty = 1100; // 1.1V * 2 cells = 2.2V = min. voltage for RFM2
bool di_change = false;
bool pin_wakeup = false; // remember if wakeup was done by pin change (or by RFM12B)

struct measurement_t
{
	int32_t val;      // stores the accumulated value
	uint16_t cnt;      // Amount of wake-up cycles counting from the last time a measurement was done.
	uint16_t measInt; // The number of times the device wakes up before this value is measured.
	uint8_t avgCnt;   // The number of values whose average is calculated before sending.
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

void init_di_sensor(void)
{
	uint8_t i;
	uint8_t measInt = e2p_envsensor_get_digitalinputmeasureinterval();
	uint8_t avgCnt = e2p_envsensor_get_digitalinputaveragingcount();
	
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

			if (di[i].pull_up)
			{
				setPullUp(di[i].port, di[i].pin);
			}

			if (di[i].mode == DIGITALINPUTMODE_ONCHANGE)
			{
				enablePCI(di[i].port, di[i].pin); // enable Pin Change Interrupt
			}

			di[i].meas.measInt = measInt;
			di[i].meas.avgCnt = avgCnt;

			UART_PUTF3("Using port %u pin %u as digital input pin %u ", di[i].port, di[i].pin, i);
			UART_PUTF2("in mode %u with pull-up %s\r\n", mode, di[i].pull_up ? "ON" : "OFF");
			
			// remember to send out status after power up
			di_change = true;
		}
	}

	if (di_change)
	{
		UART_PUTF2("(MeasInt %u, AvgCnt %u)\r\n\r\n", measInt, avgCnt);
	}
}

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

	/*
	01111111010 // ~2000ms
	10011111010 // ~4000ms
	b11100110000 // ~6s
	10111111010 // ~8000ms
	11010011100 // ~9984ms
	11011101010 // ~14976ms
	11110011100 // ~19968ms
	b11111000000 // ~24576ms
	11111101011 // ~30080ms
	100010110000 // ~45056ms
	0100101110101 // ~59904ms
	100110010010 // ~74752ms
	100110110000 // ~90112ms
	100111001101 // ~104960ms
	100111101010 // ~119808ms
	101010110000 // ~180224ms = 3m
	101011101010 // ~239616ms = 4m
	101110010010 // ~299008ms = 5m
	101111101010 // ~479232ms = 8m
	110010110000 // ~720896ms = 12m
	110011011100 // ~901120ms = 15m
	110110010010 // ~1196032ms = 20m
	*/

// Read wakeup timer value from e2p, config rfm12 and
// return the value (in seconds).
uint16_t init_wakeup(void)
{
	uint16_t interval = e2p_envsensor_get_wakeupinterval();
	
	if (interval == 0) // misconficuration in E2P
	{
		interval = WAKEUPINTERVAL_105S;
	}
	
	rfm12_set_wakeup_timer(interval);
	
	// calculate wake up time in seconds according RFM12B datasheet, round the value to seconds
	uint16_t sec = (uint16_t)(((interval & 0xff) * power(2, (interval >> 8) & 0b11111) + 500) / 1000);
	UART_PUTF("Wake-up interval: %us\r\n", sec);
	
	return sec;
}

// ---------- functions to measure values from sensors ----------

void measure_digital_input(void)
{
	if (!pin_wakeup)
	{
		di[0].meas.cnt++; // only use pin1 cnt, measInt and avgThr for all pins

		if ((di[0].meas.cnt % di[0].meas.measInt) != 0)
			return;
	}

	uint8_t i;

	for (i = 0; i < 8; i++)
	{
		if (di[i].port != DI_UNUSED)
		{
			uint8_t stat = getPinStatus(di[i].port, di[i].pin);
			
			di[i].meas.cnt++;
			
			// if status changed in OnChange mode, remember to send immediately
			if ((di[i].mode == DIGITALINPUTMODE_ONCHANGE) && (di[i].meas.val != stat))
			{
				//UART_PUTS("Status change -> send\r\n");
				di_change = true;
			}
			
			di[i].meas.val = stat; // TODO: Add averaging feature?
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
	temperature.cnt++;

	if ((temperature.cnt % temperature.measInt) != 0)
		return;

	if (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
	{
		lm75_wakeup();
		_delay_ms(lm75_get_meas_time_ms());
		temperature.val += lm75_get_tmp();
		lm75_shutdown();
	}
	else if (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085)
	{
		temperature.val += bmp085_meas_temp();
	}
}

void measure_temperature_other(void)
{
	temperature.cnt++;

	if ((temperature.cnt % temperature.measInt) != 0)
		return;

	if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT15)
	{
		sht11_measure_loop();
		temperature.val += sht11_get_tmp();
	}
}

void measure_humidity_other(void)
{
	humidity.cnt++;

	if ((humidity.cnt % humidity.measInt) != 0)
		return;

	if (humidity_sensor_type == HUMIDITYSENSORTYPE_SHT15)
	{
		// actually measure if not done already while getting temperature
		if (temperature_sensor_type != TEMPERATURESENSORTYPE_SHT15)
		{
			sht11_measure_loop();
		}
	
		humidity.val += sht11_get_hum();
	}
}

void measure_barometric_pressure_i2c(void)
{
	barometric_pressure.cnt++;

	if ((barometric_pressure.cnt % barometric_pressure.measInt) != 0)
		return;

	if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
	{
		barometric_pressure.val += bmp085_meas_pressure();
	}
}

void measure_distance_i2c(void)
{
	distance.cnt++;

	if ((distance.cnt % distance.measInt) != 0)
		return;

	if (distance_sensor_type == DISTANCESENSORTYPE_SRF02)
	{
		distance.val = srf02_get_distance();
		//UART_PUTF("Dist = %d cm\r\n", distance.val);
	}
}

void measure_battery_voltage(void)
{
	battery_voltage.cnt++;

	if ((battery_voltage.cnt % battery_voltage.measInt) != 0)
		return;

	battery_voltage.val += (int)((long)read_adc(0) * 34375 / 10000 / 2); // 1.1 * 480 Ohm / 150 Ohm / 1,024
}

void measure_brightness(void)
{
	brightness.cnt++;

	if ((brightness.cnt % brightness.measInt) != 0)
		return;

	if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
	{
		brightness.val += read_adc(1);
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
	
	version_cnt = 0;
}

// ---------- main loop ----------

int main(void)
{
	uint16_t device_id = 0;
	uint16_t wakeup_sec;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	check_eeprom_compatibility(DEVICETYPE_ENVSENSOR);

	osccal_init();
	uart_init();

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
	
	UART_PUTS ("\r\n");
	UART_PUTF4("smarthomatic EnvSensor v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	UART_PUTS("(c) 2012..2014 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	
	init_di_sensor();
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	rfm12_init();
	wakeup_sec = init_wakeup();

	// Configure measurement and averaging intervals.
	temperature.measInt = e2p_envsensor_get_temperaturemeasureinterval();
	humidity.measInt = e2p_envsensor_get_humiditymeasureinterval();
	barometric_pressure.measInt = e2p_envsensor_get_barometricmeasureinterval();
	brightness.measInt = e2p_envsensor_get_brightnessmeasureinterval();
	distance.measInt = e2p_envsensor_get_distancemeasureinterval();
	battery_voltage.measInt = 12000 / wakeup_sec;
	version_measInt = 85000 / wakeup_sec;
	version_cnt = version_measInt - 1; // send right after startup
	
	temperature.avgCnt = e2p_envsensor_get_temperatureaveragingcount();
	humidity.avgCnt = e2p_envsensor_get_humidityaveragingcount();
	barometric_pressure.avgCnt = e2p_envsensor_get_barometricaveragingcount();
	brightness.avgCnt = e2p_envsensor_get_brightnessaveragingcount();
	distance.avgCnt = e2p_envsensor_get_distanceaveragingcount();
	battery_voltage.avgCnt = 8;

	UART_PUTF3("Temperature sensor type: %u (MeasInt %u, AvgCnt %u)\r\n", temperature_sensor_type, temperature.measInt, temperature.avgCnt);
	UART_PUTF3("Humidity sensor type: %u (MeasInt %u, AvgCnt %u)\r\n", humidity_sensor_type, humidity.measInt, humidity.avgCnt);
	UART_PUTF3("Barometric sensor type: %u (MeasInt %u, AvgCnt %u)\r\n", barometric_sensor_type, barometric_pressure.measInt, barometric_pressure.avgCnt);
	UART_PUTF3("Brightness sensor type: %u (MeasInt %u, AvgCnt %u)\r\n", brightness_sensor_type, brightness.measInt, brightness.avgCnt);
	UART_PUTF3("Distance sensor type: %u (MeasInt %u, AvgCnt %u)\r\n", distance_sensor_type, distance.measInt, distance.avgCnt);

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
	
	UART_PUTF3("Min. battery voltage: %umV (MeasInt %u, AvgCnt %u)\r\n", vempty, battery_voltage.measInt, battery_voltage.avgCnt);

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
			bool measure_srf02 = srf02_connected && (distance.cnt + 1 == distance.avgCnt); // SRF02 only measures every avgCnt cycles!
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
			
			version_cnt++;
		}

		// search for value to send with avgCnt reached
		bool send = true;
		
		if (di_change)
		{
			prepare_digitalpin();
		}
		else if (humidity.cnt >= humidity.measInt * humidity.avgCnt)
		{
			prepare_humiditytemperature();
		}
		else if (barometric_pressure.cnt >= barometric_pressure.measInt * barometric_pressure.avgCnt)
		{
			prepare_barometricpressuretemperature();
		}
		else if (temperature.cnt >= temperature.measInt * temperature.avgCnt)
		{
			prepare_temperature();
		}
		else if (distance.cnt >= distance.measInt * distance.avgCnt)
		{
			prepare_distance();
		}
		else if (brightness.cnt >= brightness.measInt * brightness.avgCnt)
		{
			prepare_brightness();
		}
		else if (battery_voltage.cnt >= battery_voltage.measInt * battery_voltage.avgCnt)
		{
			prepare_battery_voltage();
		}
		else if (version_cnt >= version_measInt)
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
