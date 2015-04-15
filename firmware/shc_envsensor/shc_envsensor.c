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
#include "sht2x_htu21d.h"
#include "onewire.h"

#include "../src_common/aes256.h"
#include "../src_common/util.h"
#include "version.h"

// Pull-up resistors that are switched on for battery and brightness measurement
#define ADC_PULLUP_DDR  DDRB
#define ADC_PULLUP_PORT PORTB
#define ADC_PULLUP_PIN  6

#define SRF02_POWER_PORT PORTD
#define SRF02_POWER_DDR DDRD
#define SRF02_POWER_PIN 5

#define VERSION_MEASURING_INTERVAL_SEC 86000 // about once a day
#define BATTERY_MEASURING_INTERVAL_SEC 28500 // about every 8 hours
#define BATTERY_AVERAGING_INTERVAL 3

uint8_t temperature_sensor_type = 0;
uint8_t humidity_sensor_type = 0;
uint8_t barometric_sensor_type = 0;
uint8_t brightness_sensor_type = 0;
uint8_t distance_sensor_type = 0;
uint32_t version_measInt;
uint32_t version_wupCnt;
uint16_t vempty = 1100; // 1.1V * 2 cells = 2.2V = min. voltage for RFM12B
uint8_t rom_id[8]; // for 1-wire

bool di_sensor_used = false;
bool ai_sensor_used = false;
bool di_change = false;
bool ai_change = false;
bool pin_wakeup = false; // remember if wakeup was done by pin change (or by RFM12B)

struct measurement_t
{
	int32_t val;      // stores the accumulated value
	uint16_t wupCnt;  // Amount of wake-up cycles counting from the last time a measurement was done.
	uint16_t measInt; // The number of times the device wakes up before this value is measured.
	uint8_t measCnt;  // The number of measurements that were taken since the last time the average was sent.
	uint8_t avgInt;   // The number of values whose average is calculated before sending.
} temperature, humidity, barometric_pressure, distance, battery_voltage, brightness;

struct di_portpin_t
{
	uint8_t port;
	uint8_t pin;
	uint8_t mode;
	bool pull_up;
	struct measurement_t meas;
};

struct ai_portpin_t
{
	uint8_t pin;
	uint8_t mode;
	uint16_t thr;
	uint16_t hyst;
	uint16_t val_ref; // used for hysteresis function
	bool over_thr;    // Reference voltage was over threshold?
	struct measurement_t meas;
};

#define DI_UNUSED 255

struct di_portpin_t di[8];
struct ai_portpin_t ai[5];

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
		sbi(PORTD, pin);
	else if (port_nr == 1)
		sbi(PORTC, pin);
	else
		sbi(PORTB, pin);
}

void clearPullUp(uint8_t port_nr, uint8_t pin)
{
	if (port_nr == 2)
		cbi(PORTD, pin);
	else if (port_nr == 1)
		cbi(PORTC, pin);
	else
		cbi(PORTB, pin);
}

// Save the DI state right before going to power down so another pin change interrupt causes a
// new message if necessary.
void remember_di_state(void)
{
	uint8_t i;
	
	for (i = 0; i < 8; i++)
	{
		if (di[i].pin != DI_UNUSED)
		{
			uint8_t stat = getPinStatus(di[i].port, di[i].pin);
			di[i].meas.val = stat;
		}
	}
}

void init_di_sensor(void)
{
	uint8_t i;
	uint8_t measInt = e2p_envsensor_get_digitalinputmeasuringinterval();
	uint8_t avgInt = e2p_envsensor_get_digitalinputaveraginginterval();
	
	for (i = 0; i < 8; i++)
	{
		uint8_t pin = e2p_envsensor_get_digitalinputpin(i);

		di[i].meas.val = 0;
		di[i].meas.wupCnt = 0;
		di[i].meas.measInt = measInt;
		di[i].meas.measCnt = 0;
		di[i].meas.avgInt = avgInt;
		
		if (pin == 0) // not used
		{
			di[i].pin = DI_UNUSED;
			di[i].pull_up = false;
		}
		else
		{
			di[i].pull_up = e2p_envsensor_get_digitalinputpullupresistor(i);
			uint8_t mode = e2p_envsensor_get_digitalinputtriggermode(i);

			di[i].port = (pin - 1) / 8;
			di[i].pin = (pin - 1) % 8;
			di[i].mode = mode;
			
			if (di[i].mode != DIGITALINPUTTRIGGERMODE_OFF)
			{
				enablePCI(di[i].port, di[i].pin); // enable Pin Change Interrupt
				
				if (di[i].pull_up)
				{
					setPullUp(di[i].port, di[i].pin); // when using PCI, pullups should be active
				}
			}

			UART_PUTF3("Using port %u pin %u as digital input pin %u ", di[i].port, di[i].pin, i);
			UART_PUTF2("in mode %u with pull-up %s\r\n", mode, di[i].pull_up ? "ON" : "OFF");
			
			di_sensor_used = true;

			// remember to send out status after power up
			di_change = true;
		}
	}

	if (di_sensor_used)
	{
		UART_PUTF2("(MeasInt %u, avgInt %u)\r\n\r\n", measInt, avgInt);
	}
}

void init_ai_sensor(void)
{
	uint8_t i;
	uint8_t measInt = e2p_envsensor_get_analoginputmeasuringinterval();
	uint8_t avgInt = e2p_envsensor_get_analoginputaveraginginterval();
	
	UART_PUTS("Init analog\r\n");
	
	for (i = 0; i < 5; i++)
	{
		uint8_t pin = e2p_envsensor_get_analoginputpin(i);

		ai[i].meas.val = 0;
		ai[i].meas.wupCnt = 0;
		ai[i].meas.measInt = measInt;
		ai[i].meas.measCnt = 0;
		ai[i].meas.avgInt = avgInt;
		
		if (pin == 0) // not used
		{
			ai[i].pin = DI_UNUSED;
		}
		else
		{
			//di[i].pull_up = e2p_envsensor_get_digitalinputpullupresistor(i);
			uint8_t mode = e2p_envsensor_get_analoginputtriggermode(i);

			ai[i].pin = (pin - 1) % 8;
			ai[i].mode = mode;
			ai[i].thr = e2p_envsensor_get_analoginputtriggerthreshold(i);
			ai[i].hyst = e2p_envsensor_get_analoginputtriggerhysteresis(i);
			ai[i].val_ref = 0;
			ai[i].over_thr = false;
			
			UART_PUTF2("Using port ADC%u as analog input pin %u ", ai[i].pin, i);
			UART_PUTF3("in mode %u with threshold %umV and hysteresis %umV\r\n", mode, ai[i].thr, ai[i].hyst);
			
			ai_sensor_used = true;

			// set DIDR for ADC channels, switch off digital input buffers to reduce ADC noise and to save power
			sbi(DIDR0, ai[i].pin);
	
			// remember to send out status after power up
			ai_change = true;
		}
	}

	if (ai_sensor_used)
	{
		UART_PUTF2("(MeasInt %u, avgInt %u)\r\n\r\n", measInt, avgInt);
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

	/* Wake-up time is calculated as T = M * 2 ^ R [ms]
	Value is saved as RRRRRMMMMMMMM
	
	   Binary       Decimal     ms    equals
	0001111111010   1018       2000      2s
	0010011111010   1274       4000      4s
	0010110111011   1467       5984     ~6s
	0010111111010   1530       8000      8s
	0011010011100   1692       9984    ~10s
	0011011101010   1770      14976    ~15s
	0011110011100   1948      19968    ~20s
	0011111000000   1984      24576    ~25s
	0011111101011   2027      30080    ~30s
	0100010110000   2224      45056    ~45s
	0100101110101   2421      59904    ~60s
	0100110010010   2450      74752    ~75s
	0100110110000   2480      90112    ~90s
	0100111001101   2509     104960   ~105s
	0100111101010   2538     119808     ~2m
	0101010110000   2736     180224     ~3m
	0101011101010   2794     239616     ~4m
	0101110010010   2962     299008     ~5m
	0101111101010   3050     479232     ~8m
	0110010110000   3248     720896    ~12m
	0110011011100   3292     901120    ~15m
	0110110010010   3474    1196032    ~20m
	0110111011100   3548    1802240    ~30m
	0111011011100   3804    3604480     ~1h
	0111111011100   4060    7208960     ~2h
	1000010100101   4261   10813440     ~3h
	1000011011100   4316   14417920     ~4h
	1000110100101   4517   21626880     ~6h
	1000111011100   4572   28835840     ~8h
	1001010100101   4773   43253760    ~12h
	1001011011100   4828   57671680    ~16h
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
	
	// Calculate wake-up time in seconds according RFM12B datasheet and round the value to seconds.
	uint16_t sec = (uint16_t)(((interval & 0xff) * power(2, (interval >> 8) & 0b11111) + 500) / 1000);
	UART_PUTF("Wake-up interval: %us\r\n", sec);
	
	return sec;
}

// ---------- functions to measure values from sensors ----------

// count wake-up and return true if the measurement interval is reached
bool countWakeup(struct measurement_t * m)
{
	m->wupCnt++;
	
	if (m->wupCnt >= m->measInt)
	{
		m->wupCnt = 0;
		m->measCnt++;
		return true;
	}
	
	return false;
}

void measure_digital_input(void)
{
	if (!di_sensor_used)
		return;

	if (!pin_wakeup)
	{
		if (!countWakeup(&di[0].meas)) // only use pin1 cnt, measInt and avgThr for all pins
			return;
	}

	uint8_t i;
	bool wait_pullups = false;
	
	for (i = 0; i < 8; i++)
	{
		if ((di[i].pull_up) && (di[i].mode == DIGITALINPUTTRIGGERMODE_OFF))
		{
			setPullUp(di[i].port, di[i].pin);
			wait_pullups = true;
		}
	}
	
	// wait a little bit to let the voltage level settle down in case pullups were just switched on
	if (wait_pullups)
	{
		_delay_ms(5);
	}
	
	for (i = 0; i < 8; i++)
	{
		if (di[i].pin != DI_UNUSED)
		{
			uint8_t stat = getPinStatus(di[i].port, di[i].pin);
			
			// if status changed in OnChange mode, remember to send immediately
			if (pin_wakeup && (di[i].meas.val != stat)
					&& ( (di[i].mode == DIGITALINPUTTRIGGERMODE_CHANGE)
					||   ((di[i].mode == DIGITALINPUTTRIGGERMODE_UP) && (stat == 1))
					||   ((di[i].mode == DIGITALINPUTTRIGGERMODE_DOWN) && (stat == 0)) ))
			{
				UART_PUTF("Pin wakeup + pin change at pin %u -> send\r\n", i);
				di_change = true;
			}
			// in cyclic measure mode, remember to send if avgInt reached
			else if (di[0].meas.measCnt >= di[0].meas.avgInt)
			{
				UART_PUTF("Pin avgCnt reached -> send\r\n", i);
				di_change = true;
			}
			
			di[i].meas.val = stat; // TODO: Add averaging feature?
		}
	}
	
	for (i = 0; i < 8; i++)
	{
		if ((di[i].pull_up) && (di[i].mode == DIGITALINPUTTRIGGERMODE_OFF))
		{
			clearPullUp(di[i].port, di[i].pin);
		}
	}
}

void measure_analog_input(void)
{
	uint8_t i;

	if (!ai_sensor_used)
		return;

	if (!countWakeup(&ai[0].meas)) // only use pin1 cnt, measInt and avgThr for all pins
		return;
	
	for (i = 0; i < 5; i++)
	{
		if (ai[i].pin != DI_UNUSED)
		{
			// Maximum ADC value of 1023 = 1.1V. Save value in 0.1 mV.
			uint16_t voltage = (uint16_t)(((uint32_t)read_adc(ai[i].pin) * 11000) / 1023);
			
			ai[i].meas.val += voltage; // accumulate values to calc average later on

			if (ai[i].mode != ANALOGINPUTTRIGGERMODE_OFF)
			{
				// Remember maximum / minimum voltage as reference for hysteresis function.
				if (ai[i].over_thr == (voltage > ai[i].val_ref))
				{
					ai[i].val_ref = voltage;
				}
				
				bool trigger_down = ai[i].over_thr && (voltage / 10 + ai[i].hyst < ai[i].val_ref / 10); // beware of overflow!
				bool trigger_up = !ai[i].over_thr && (voltage / 10 > ai[i].val_ref / 10 + ai[i].hyst);
				bool trigger = trigger_down || trigger_up;
				
				if (trigger)
				{
					// When a trigger fires, reverse the direction for the next trigger.
					ai[i].over_thr = !ai[i].over_thr;
					ai[i].val_ref = voltage;
					
					// Check if the value has to be send (depending on the configuration).
					if ( (trigger_down && (ai[i].mode == ANALOGINPUTTRIGGERMODE_DOWN))
						|| (trigger_up && (ai[i].mode == ANALOGINPUTTRIGGERMODE_UP))
						|| (ai[i].mode == ANALOGINPUTTRIGGERMODE_CHANGE))
					{
						ai_change = true;
						
						// use only current value instead of average
						ai[i].meas.val = voltage;
						ai[i].meas.measCnt = 1;
					}
				}
			}
			
			//UART_PUTF2("voltage: %u, maes.val: %u, ", voltage, ai[i].meas.val);
			//UART_PUTF3("val_ref: %u, over_thr: %u, ai_change: %u", ai[i].val_ref, ai[i].over_thr, ai_change);
			//UART_PUTS("\r\n\r\n");
		}
	}

	// remember to send when cycle count reached
	if (ai[0].meas.measCnt >= ai[0].meas.avgInt)
	{
		ai_change = true;
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
	if ((temperature_sensor_type != TEMPERATURESENSORTYPE_DS7505)
		&& (temperature_sensor_type != TEMPERATURESENSORTYPE_BMP085)
		&& (temperature_sensor_type != TEMPERATURESENSORTYPE_SHT2X_HTU21D))
		return;

	if (!countWakeup(&temperature))
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
	else if (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT2X_HTU21D)
	{
		temperature.val += sht2x_htu21d_meas_temp();
	}
}

void measure_temperature_1wire(void)
{
	if (temperature_sensor_type != TEMPERATURESENSORTYPE_DS18X20)
		return;
		
	if (!countWakeup(&temperature))
		return;

	temperature.val += onewire_get_temperature(rom_id);
}

void measure_temperature_other(void)
{
	if (temperature_sensor_type != TEMPERATURESENSORTYPE_SHT15)
		return;

	if (!countWakeup(&temperature))
		return;

	sht11_measure_loop();
	temperature.val += sht11_get_tmp();
}

void measure_humidity_i2c(void)
{
	if (humidity_sensor_type != HUMIDITYSENSORTYPE_SHT2X_HTU21D)
		return;

	if (!countWakeup(&humidity))
		return;

	humidity.val += sht2x_htu21d_meas_hum();
}

void measure_humidity_other(void)
{
	if (humidity_sensor_type != HUMIDITYSENSORTYPE_SHT15)
		return;

	if (!countWakeup(&humidity))
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
	if (barometric_sensor_type == BAROMETRICSENSORTYPE_NOSENSOR)
		return;

	if (!countWakeup(&barometric_pressure))
		return;

	if (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
	{
		barometric_pressure.val += bmp085_meas_pressure();
	}
}

void measure_distance_i2c(void)
{
	if (distance_sensor_type == DISTANCESENSORTYPE_NOSENSOR)
		return;

	if (!countWakeup(&distance))
		return;

	if (distance_sensor_type == DISTANCESENSORTYPE_SRF02)
	{
		distance.val += srf02_get_distance();
		//UART_PUTF("Dist sum = %u cm\r\n", distance.val);
	}
}

void measure_battery_voltage(void)
{
	if (!countWakeup(&battery_voltage))
		return;

	battery_voltage.val += read_battery();
}

void measure_brightness(void)
{
	if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_NOSENSOR)
		return;

	if (!countWakeup(&brightness))
		return;

	if (brightness_sensor_type == BRIGHTNESSSENSORTYPE_PHOTOCELL)
	{
		brightness.val += read_adc(1);
	}
}

// ---------- functions to prepare a message filled with sensor data ----------

// Calculate the average value and reset wake-up and measurement counter.
void average(struct measurement_t *m)
{
	m->val /= m->measCnt;
	m->wupCnt = 0;
	m->measCnt = 0;
}

void prepare_digitalport(void)
{
	pkg_header_init_gpio_digitalport_status();

	UART_PUTS("Send DigitalPort: ");
	
	uint8_t i;
	
	for (i = 0; i < 8; i++)
	{
		
		if (di[i].pin != DI_UNUSED)
		{
			UART_PUTF("%u", di[i].meas.val);
			
			msg_gpio_digitalport_set_on(i, di[i].meas.val != 0);
		}
		else
		{
			UART_PUTS("-");
		}
	}

	UART_PUTS("\r\n");
	
	di[0].meas.wupCnt = 0;
	di[0].meas.measCnt = 0;
	di_change = false;
}

void prepare_analogport(void)
{
	pkg_header_init_gpio_analogport_status();

	UART_PUTS("Send AnalogPort:");
	
	uint8_t i;
	
	for (i = 0; i < 5; i++)
	{
		if (ai[i].pin != DI_UNUSED)
		{
			average(&ai[i].meas);
			ai[i].meas.val = (ai[i].meas.val + 5) / 10; // round to mV

			bool on = ((ai[i].mode == ANALOGINPUTTRIGGERMODE_OFF) && (ai[i].meas.val >= ai[i].thr))
				|| ((ai[i].mode != ANALOGINPUTTRIGGERMODE_OFF) && ai[i].over_thr);
		
			UART_PUTF2(" %u/%u", ai[i].meas.val, on);
			
			msg_gpio_analogport_set_on(i, on);
			msg_gpio_analogport_set_voltage(i, ai[i].meas.val);
			
			ai[i].meas.val = 0;
		}
		else
		{
			UART_PUTS(" -");
		}
	}
	
	ai[0].meas.wupCnt = 0;
	UART_PUTS("\r\n");

	ai_change = false;
}

void prepare_humiditytemperature(void)
{
	average(&humidity); // in 100 * % rel.
	average(&temperature);

	pkg_header_init_weather_humiditytemperature_status();
	msg_weather_humiditytemperature_set_humidity(humidity.val / 10); // in permill
	msg_weather_humiditytemperature_set_temperature(temperature.val);

	UART_PUTF2("Send humidity: %u.%u%%, temperature: ", (uint16_t)(humidity.val / 100), (uint16_t)(humidity.val % 100));
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");

	humidity.val = 0;
	temperature.val = 0;
}

void prepare_barometricpressuretemperature(void)
{
	average(&barometric_pressure);
	average(&temperature);

	pkg_header_init_weather_barometricpressuretemperature_status();
	msg_weather_barometricpressuretemperature_set_barometricpressure(barometric_pressure.val);
	msg_weather_barometricpressuretemperature_set_temperature(temperature.val);

	UART_PUTF("Send barometric pressure: %ld pascal, temperature: ", barometric_pressure.val);
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");
	
	barometric_pressure.val = 0;
	temperature.val = 0;
}

void prepare_temperature(void)
{
	average(&temperature);
	
	pkg_header_init_weather_temperature_status();
	msg_weather_temperature_set_temperature(temperature.val);
	
	UART_PUTS("Send temperature: ");
	print_signed(temperature.val);
	UART_PUTS(" deg.C\r\n");
	
	temperature.val = 0;
}

void prepare_distance(void)
{
	average(&distance);

	pkg_header_init_environment_distance_status();
	msg_environment_distance_set_distance(distance.val);
	
	UART_PUTF("Send distance: %d\r\n", distance.val);
	
	distance.val = 0;
}
void prepare_brightness(void)
{
	average(&brightness);
	brightness.val = 100 - (int)((long)brightness.val * 100 / 1024);

	pkg_header_init_environment_brightness_status();
	msg_environment_brightness_set_brightness(brightness.val);
	
	UART_PUTF("Send brightness: %u%%\r\n", brightness.val);
	
	brightness.val = 0;
}

void prepare_battery_voltage(void)
{
	average(&battery_voltage);
	battery_voltage.val = bat_percentage(battery_voltage.val / 2, vempty);

	pkg_header_init_generic_batterystatus_status();
	msg_generic_batterystatus_set_percentage(battery_voltage.val);
				
	UART_PUTF("Send battery: %u%%\r\n", battery_voltage.val);

	battery_voltage.val = 0;
}

void prepare_deviceinfo(void)
{
	// Set packet content
	pkg_header_init_generic_deviceinfo_status();
	msg_generic_deviceinfo_set_devicetype(DEVICETYPE_ENVSENSOR);
	msg_generic_deviceinfo_set_versionmajor(VERSION_MAJOR);
	msg_generic_deviceinfo_set_versionminor(VERSION_MINOR);
	msg_generic_deviceinfo_set_versionpatch(VERSION_PATCH);
	msg_generic_deviceinfo_set_versionhash(VERSION_HASH);

	UART_PUTF("Send DeviceInfo: DeviceType %u,", DEVICETYPE_ENVSENSOR);
	UART_PUTF4(" v%u.%u.%u (%08lx)\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_HASH);
	
	version_wupCnt = 0;
}

// ---------- main loop ----------

int main(void)
{
	uint16_t device_id = 0;
	uint16_t wakeup_sec;

	// delay 1s to avoid further communication with uart or RFM12 when my programmer resets the MC after 500ms...
	_delay_ms(1000);

	util_init();

	sbi(ADC_PULLUP_DDR, ADC_PULLUP_PIN);
	
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
	UART_PUTS("(c) 2012..2015 Uwe Freese, www.smarthomatic.org\r\n");
	osccal_info();
	UART_PUTF ("Device ID: %u\r\n", device_id);
	UART_PUTF ("Packet counter: %lu\r\n", packetcounter);
	
	init_di_sensor();
	init_ai_sensor();
	
	// init AES key
	e2p_generic_get_aeskey(aes_key);

	rfm12_init();
	wakeup_sec = init_wakeup();

	// Configure measurement and averaging intervals.
	temperature.measInt = e2p_envsensor_get_temperaturemeasuringinterval();
	humidity.measInt = e2p_envsensor_get_humiditymeasuringinterval();
	barometric_pressure.measInt = e2p_envsensor_get_barometricmeasuringinterval();
	brightness.measInt = e2p_envsensor_get_brightnessmeasuringinterval();
	distance.measInt = e2p_envsensor_get_distancemeasuringinterval();
	battery_voltage.measInt = BATTERY_MEASURING_INTERVAL_SEC / wakeup_sec;
	version_measInt = VERSION_MEASURING_INTERVAL_SEC / wakeup_sec;
	version_wupCnt = version_measInt - 1; // send right after startup
	
	temperature.avgInt = e2p_envsensor_get_temperatureaveraginginterval();
	humidity.avgInt = e2p_envsensor_get_humidityaveraginginterval();
	barometric_pressure.avgInt = e2p_envsensor_get_barometricaveraginginterval();
	brightness.avgInt = e2p_envsensor_get_brightnessaveraginginterval();
	distance.avgInt = e2p_envsensor_get_distanceaveraginginterval();
	battery_voltage.avgInt = BATTERY_AVERAGING_INTERVAL;

	UART_PUTF3("Temperature sensor type: %u (measInt %u, avgInt %u)\r\n", temperature_sensor_type, temperature.measInt, temperature.avgInt);
	UART_PUTF3("Humidity sensor type: %u (measInt %u, avgInt %u)\r\n", humidity_sensor_type, humidity.measInt, humidity.avgInt);
	UART_PUTF3("Barometric sensor type: %u (measInt %u, avgInt %u)\r\n", barometric_sensor_type, barometric_pressure.measInt, barometric_pressure.avgInt);
	UART_PUTF3("Brightness sensor type: %u (measInt %u, avgInt %u)\r\n", brightness_sensor_type, brightness.measInt, brightness.avgInt);
	UART_PUTF3("Distance sensor type: %u (measInt %u, avgInt %u)\r\n", distance_sensor_type, distance.measInt, distance.avgInt);

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
	else if (temperature_sensor_type == TEMPERATURESENSORTYPE_DS18X20)
	{
		onewire_init();
		bool res = onewire_get_rom_id(rom_id);
		
		if (res) // error, no slave found
		{
			while (1)
			{
				led_blink(50, 50, 1);
			}
		}
		
		UART_PUTS("1-wire ROM ID: ");
		print_bytearray(rom_id, 8);
		
		vempty = 1500; // 1.5V * 2 cells = 3.0V = min. voltage for DS18X20
	}
	
	UART_PUTF3("Min. battery voltage: %umV (measInt %u, avgInt %u)\r\n", vempty, battery_voltage.measInt, battery_voltage.avgInt);

	if ((barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
		|| (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085))
	{
		i2c_enable();
		bmp085_init();
		i2c_disable();
	}

	if (distance_sensor_type == DISTANCESENSORTYPE_SRF02)
	{
		sbi(SRF02_POWER_DDR, SRF02_POWER_PIN);
	}

	UART_PUTS("\r\n");

	led_blink(500, 500, 3);
	
	sei();

	// If a SRF02 is connected, it is assumed that it is powered by a step up voltage converter,
	// which produces 5V out of the ~3V battery power. It has to be switched on to activate
	// the SRF02 and also make it possible to communicate to other I2C devices.

	bool srf02_connected = distance_sensor_type == DISTANCESENSORTYPE_SRF02;
	bool measure_other_i2c = (temperature_sensor_type == TEMPERATURESENSORTYPE_DS7505)
		|| (temperature_sensor_type == TEMPERATURESENSORTYPE_BMP085)
		|| (barometric_sensor_type == BAROMETRICSENSORTYPE_BMP085)
		|| (temperature_sensor_type == TEMPERATURESENSORTYPE_SHT2X_HTU21D)
		|| (humidity_sensor_type == HUMIDITYSENSORTYPE_SHT2X_HTU21D);

	while (42)
	{
		if (pin_wakeup)
		{
			measure_digital_input();
		}
		else // wakeup by RFM12B -> measure everything
		{
			bool measure_srf02 = srf02_connected && ((distance.wupCnt + 1) >= distance.measInt);
			bool measure_i2c = measure_srf02 || measure_other_i2c;
			bool needs_power = measure_srf02 || (measure_other_i2c && srf02_connected);

			// measure ADC dependant values
			adc_on(true);
			sbi(ADC_PULLUP_PORT, ADC_PULLUP_PIN);
			_delay_ms(1);
			measure_battery_voltage();
			measure_brightness();
			measure_analog_input();
			cbi(ADC_PULLUP_PORT, ADC_PULLUP_PIN);
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
				measure_humidity_i2c();
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
				distance.wupCnt++; // increase distance counter, because measure_distance_i2c() was not called
			}
			
			// measure other values, non-I2C devices
			measure_temperature_1wire();
			measure_temperature_other();
			measure_humidity_other();
			
			version_wupCnt++;
		}

		// search for value to send with avgInt reached
		bool send = true;
		
		if (pin_wakeup && !di_change) // don't send update if pin level changed in "wrong" direction
		{
			send = false;
		}
		else if (di_change)
		{
			prepare_digitalport();
		}
		else if (ai_change)
		{
			prepare_analogport();
		}
		else if (humidity.measCnt >= humidity.avgInt)
		{
			prepare_humiditytemperature();
		}
		else if (barometric_pressure.measCnt >= barometric_pressure.avgInt)
		{
			prepare_barometricpressuretemperature();
		}
		else if (temperature.measCnt >= temperature.avgInt)
		{
			prepare_temperature();
		}
		else if (distance.measCnt >= distance.avgInt)
		{
			prepare_distance();
		}
		else if (brightness.measCnt >= brightness.avgInt)
		{
			prepare_brightness();
		}
		else if (battery_voltage.measCnt >= battery_voltage.avgInt)
		{
			prepare_battery_voltage();
		}
		else if (version_wupCnt >= version_measInt)
		{
			prepare_deviceinfo();
		}
		else
		{
			send = false;
		}
		
		if (send)
		{
			inc_packetcounter();
			
			pkg_header_set_senderid(device_id);
			pkg_header_set_packetcounter(packetcounter);
			
			rfm12_send_bufx();
			rfm12_tick(); // send packet, and then WAIT SOME TIME BEFORE GOING TO SLEEP (otherwise packet would not be sent)

			led_blink(200, 0, 1);
		}

		cli();
		pin_wakeup = false;
		remember_di_state();
		power_down(true); // will enable interrupts again
	}
}
