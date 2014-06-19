/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013..2014 Uwe Freese
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
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* ! WARNING: This file is generated by the SHC EEPROM editor and should !
* ! never be modified manually.                                         !
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#ifndef _E2P_ENVSENSOR_H
#define _E2P_ENVSENSOR_H

#include "e2p_access.h"

// E2P Block "EnvSensor"
// =====================
// Start offset (bit): 512
// Overall block length: 7680 bits

// TemperatureSensorType (EnumValue)
// Description: You can choose one of the supported temperature / humidity sensors.

typedef enum {
  TEMPERATURESENSORTYPE_NOSENSOR = 0,
  TEMPERATURESENSORTYPE_SHT15 = 1,
  TEMPERATURESENSORTYPE_DS7505 = 2,
  TEMPERATURESENSORTYPE_BMP085 = 3
} TemperatureSensorTypeEnum;

// Set TemperatureSensorType (EnumValue)
// Byte offset: 64, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_temperaturesensortype(TemperatureSensorTypeEnum val)
{
  eeprom_write_UIntValue(64, 0, 8, val);
}

// Get TemperatureSensorType (EnumValue)
// Byte offset: 64, bit offset: 0, length bits 8
static inline TemperatureSensorTypeEnum e2p_envsensor_get_temperaturesensortype(void)
{
  return eeprom_read_UIntValue8(64, 0, 8, 0, 255);
}

// HumiditySensorType (EnumValue)
// Description: You can choose one of the supported air humidity sensors.

typedef enum {
  HUMIDITYSENSORTYPE_NOSENSOR = 0,
  HUMIDITYSENSORTYPE_SHT15 = 1
} HumiditySensorTypeEnum;

// Set HumiditySensorType (EnumValue)
// Byte offset: 65, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_humiditysensortype(HumiditySensorTypeEnum val)
{
  eeprom_write_UIntValue(65, 0, 8, val);
}

// Get HumiditySensorType (EnumValue)
// Byte offset: 65, bit offset: 0, length bits 8
static inline HumiditySensorTypeEnum e2p_envsensor_get_humiditysensortype(void)
{
  return eeprom_read_UIntValue8(65, 0, 8, 0, 255);
}

// BarometricSensorType (EnumValue)
// Description: You can choose one of the supported barometric pressure sensors.

typedef enum {
  BAROMETRICSENSORTYPE_NOSENSOR = 0,
  BAROMETRICSENSORTYPE_BMP085 = 1
} BarometricSensorTypeEnum;

// Set BarometricSensorType (EnumValue)
// Byte offset: 66, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_barometricsensortype(BarometricSensorTypeEnum val)
{
  eeprom_write_UIntValue(66, 0, 8, val);
}

// Get BarometricSensorType (EnumValue)
// Byte offset: 66, bit offset: 0, length bits 8
static inline BarometricSensorTypeEnum e2p_envsensor_get_barometricsensortype(void)
{
  return eeprom_read_UIntValue8(66, 0, 8, 0, 255);
}

// BrightnessSensorType (EnumValue)
// Description: You can choose one of the supported light sensors.

typedef enum {
  BRIGHTNESSSENSORTYPE_NOSENSOR = 0,
  BRIGHTNESSSENSORTYPE_PHOTOCELL = 1
} BrightnessSensorTypeEnum;

// Set BrightnessSensorType (EnumValue)
// Byte offset: 67, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_brightnesssensortype(BrightnessSensorTypeEnum val)
{
  eeprom_write_UIntValue(67, 0, 8, val);
}

// Get BrightnessSensorType (EnumValue)
// Byte offset: 67, bit offset: 0, length bits 8
static inline BrightnessSensorTypeEnum e2p_envsensor_get_brightnesssensortype(void)
{
  return eeprom_read_UIntValue8(67, 0, 8, 0, 255);
}

// DistanceSensorType (EnumValue)
// Description: Choose one of the connected distance sensor types.

typedef enum {
  DISTANCESENSORTYPE_NOSENSOR = 0,
  DISTANCESENSORTYPE_SRF02 = 1
} DistanceSensorTypeEnum;

// Set DistanceSensorType (EnumValue)
// Byte offset: 68, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_distancesensortype(DistanceSensorTypeEnum val)
{
  eeprom_write_UIntValue(68, 0, 8, val);
}

// Get DistanceSensorType (EnumValue)
// Byte offset: 68, bit offset: 0, length bits 8
static inline DistanceSensorTypeEnum e2p_envsensor_get_distancesensortype(void)
{
  return eeprom_read_UIntValue8(68, 0, 8, 0, 255);
}

// Reserved area with 472 bits

// WakeupInterval (EnumValue)
// Description: Decide after which time the device should be woken up by the RFM12B transceiver to measure or send values.

typedef enum {
  WAKEUPINTERVAL_2S = 1018,
  WAKEUPINTERVAL_4S = 1274,
  WAKEUPINTERVAL_6S = 1840,
  WAKEUPINTERVAL_8S = 1530,
  WAKEUPINTERVAL_10S = 1692,
  WAKEUPINTERVAL_15S = 1770,
  WAKEUPINTERVAL_20S = 1948,
  WAKEUPINTERVAL_30S = 2027,
  WAKEUPINTERVAL_45S = 2224,
  WAKEUPINTERVAL_60S = 2421,
  WAKEUPINTERVAL_75S = 2450,
  WAKEUPINTERVAL_90S = 2480,
  WAKEUPINTERVAL_105S = 2509,
  WAKEUPINTERVAL_120S = 2538,
  WAKEUPINTERVAL_3M = 2736,
  WAKEUPINTERVAL_4M = 2794,
  WAKEUPINTERVAL_5M = 2962,
  WAKEUPINTERVAL_8M = 3050,
  WAKEUPINTERVAL_12M = 3248,
  WAKEUPINTERVAL_15M = 3292,
  WAKEUPINTERVAL_20M = 3474
} WakeupIntervalEnum;

// Set WakeupInterval (EnumValue)
// Byte offset: 128, bit offset: 0, length bits 16
static inline void e2p_envsensor_set_wakeupinterval(WakeupIntervalEnum val)
{
  eeprom_write_UIntValue(128, 0, 16, val);
}

// Get WakeupInterval (EnumValue)
// Byte offset: 128, bit offset: 0, length bits 16
static inline WakeupIntervalEnum e2p_envsensor_get_wakeupinterval(void)
{
  return eeprom_read_UIntValue16(128, 0, 16, 0, 65535);
}

// TemperatureMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set TemperatureMeasuringInterval (UIntValue)
// Byte offset: 130, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_temperaturemeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(130, 0, 8, val);
}

// Get TemperatureMeasuringInterval (UIntValue)
// Byte offset: 130, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_temperaturemeasuringinterval(void)
{
  return eeprom_read_UIntValue8(130, 0, 8, 1, 255);
}

// TemperatureAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set TemperatureAveragingInterval (UIntValue)
// Byte offset: 131, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_temperatureaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(131, 0, 8, val);
}

// Get TemperatureAveragingInterval (UIntValue)
// Byte offset: 131, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_temperatureaveraginginterval(void)
{
  return eeprom_read_UIntValue8(131, 0, 8, 1, 16);
}

// HumidityMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set HumidityMeasuringInterval (UIntValue)
// Byte offset: 132, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_humiditymeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(132, 0, 8, val);
}

// Get HumidityMeasuringInterval (UIntValue)
// Byte offset: 132, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_humiditymeasuringinterval(void)
{
  return eeprom_read_UIntValue8(132, 0, 8, 1, 255);
}

// HumidityAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set HumidityAveragingInterval (UIntValue)
// Byte offset: 133, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_humidityaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(133, 0, 8, val);
}

// Get HumidityAveragingInterval (UIntValue)
// Byte offset: 133, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_humidityaveraginginterval(void)
{
  return eeprom_read_UIntValue8(133, 0, 8, 1, 16);
}

// BarometricMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set BarometricMeasuringInterval (UIntValue)
// Byte offset: 134, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_barometricmeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(134, 0, 8, val);
}

// Get BarometricMeasuringInterval (UIntValue)
// Byte offset: 134, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_barometricmeasuringinterval(void)
{
  return eeprom_read_UIntValue8(134, 0, 8, 1, 255);
}

// BarometricAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set BarometricAveragingInterval (UIntValue)
// Byte offset: 135, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_barometricaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(135, 0, 8, val);
}

// Get BarometricAveragingInterval (UIntValue)
// Byte offset: 135, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_barometricaveraginginterval(void)
{
  return eeprom_read_UIntValue8(135, 0, 8, 1, 16);
}

// BrightnessMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set BrightnessMeasuringInterval (UIntValue)
// Byte offset: 136, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_brightnessmeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(136, 0, 8, val);
}

// Get BrightnessMeasuringInterval (UIntValue)
// Byte offset: 136, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_brightnessmeasuringinterval(void)
{
  return eeprom_read_UIntValue8(136, 0, 8, 1, 255);
}

// BrightnessAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set BrightnessAveragingInterval (UIntValue)
// Byte offset: 137, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_brightnessaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(137, 0, 8, val);
}

// Get BrightnessAveragingInterval (UIntValue)
// Byte offset: 137, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_brightnessaveraginginterval(void)
{
  return eeprom_read_UIntValue8(137, 0, 8, 1, 16);
}

// DistanceMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set DistanceMeasuringInterval (UIntValue)
// Byte offset: 138, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_distancemeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(138, 0, 8, val);
}

// Get DistanceMeasuringInterval (UIntValue)
// Byte offset: 138, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_distancemeasuringinterval(void)
{
  return eeprom_read_UIntValue8(138, 0, 8, 1, 255);
}

// DistanceAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set DistanceAveragingInterval (UIntValue)
// Byte offset: 139, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_distanceaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(139, 0, 8, val);
}

// Get DistanceAveragingInterval (UIntValue)
// Byte offset: 139, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_distanceaveraginginterval(void)
{
  return eeprom_read_UIntValue8(139, 0, 8, 1, 16);
}

// DigitalInputMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set DigitalInputMeasuringInterval (UIntValue)
// Byte offset: 140, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_digitalinputmeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(140, 0, 8, val);
}

// Get DigitalInputMeasuringInterval (UIntValue)
// Byte offset: 140, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_digitalinputmeasuringinterval(void)
{
  return eeprom_read_UIntValue8(140, 0, 8, 1, 255);
}

// DigitalInputAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set DigitalInputAveragingInterval (UIntValue)
// Byte offset: 141, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_digitalinputaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(141, 0, 8, val);
}

// Get DigitalInputAveragingInterval (UIntValue)
// Byte offset: 141, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_digitalinputaveraginginterval(void)
{
  return eeprom_read_UIntValue8(141, 0, 8, 1, 16);
}

// AnalogInputMeasuringInterval (UIntValue)
// Description: The number of times the device wakes up before this value is measured.

// Set AnalogInputMeasuringInterval (UIntValue)
// Byte offset: 142, bit offset: 0, length bits 8, min val 1, max val 255
static inline void e2p_envsensor_set_analoginputmeasuringinterval(uint8_t val)
{
  eeprom_write_UIntValue(142, 0, 8, val);
}

// Get AnalogInputMeasuringInterval (UIntValue)
// Byte offset: 142, bit offset: 0, length bits 8, min val 1, max val 255
static inline uint8_t e2p_envsensor_get_analoginputmeasuringinterval(void)
{
  return eeprom_read_UIntValue8(142, 0, 8, 1, 255);
}

// AnalogInputAveragingInterval (UIntValue)
// Description: The number of values whose average is calculated before sending.

// Set AnalogInputAveragingInterval (UIntValue)
// Byte offset: 143, bit offset: 0, length bits 8, min val 1, max val 16
static inline void e2p_envsensor_set_analoginputaveraginginterval(uint8_t val)
{
  eeprom_write_UIntValue(143, 0, 8, val);
}

// Get AnalogInputAveragingInterval (UIntValue)
// Byte offset: 143, bit offset: 0, length bits 8, min val 1, max val 16
static inline uint8_t e2p_envsensor_get_analoginputaveraginginterval(void)
{
  return eeprom_read_UIntValue8(143, 0, 8, 1, 16);
}

// Reserved area with 384 bits

// DigitalInputPins (EnumValue[8])
// Description: You can choose up to 8 GPIO pins as digital input. The enum values are couting through every pin from port B, C and D, leaving out the pins that are not accessible because otherwise used.

typedef enum {
  DIGITALINPUTPINS_UNUSED = 0,
  DIGITALINPUTPINS_PB1 = 2,
  DIGITALINPUTPINS_PB2 = 3,
  DIGITALINPUTPINS_PB6 = 7,
  DIGITALINPUTPINS_PB7 = 8,
  DIGITALINPUTPINS_PC1 = 10,
  DIGITALINPUTPINS_PC2 = 11,
  DIGITALINPUTPINS_PC3 = 12,
  DIGITALINPUTPINS_PC4 = 13,
  DIGITALINPUTPINS_PC5 = 14,
  DIGITALINPUTPINS_PD3 = 20,
  DIGITALINPUTPINS_PD4 = 21,
  DIGITALINPUTPINS_PD5 = 22,
  DIGITALINPUTPINS_PD6 = 23
} DigitalInputPinsEnum;

// Set DigitalInputPins (EnumValue)
// Byte offset: 192, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_digitalinputpins(uint8_t index, DigitalInputPinsEnum val)
{
  eeprom_write_UIntValue(192 + (uint16_t)index * 1, 0, 8, val);
}

// Get DigitalInputPins (EnumValue)
// Byte offset: 192, bit offset: 0, length bits 8
static inline DigitalInputPinsEnum e2p_envsensor_get_digitalinputpins(uint8_t index)
{
  return eeprom_read_UIntValue8(192 + (uint16_t)index * 1, 0, 8, 0, 255);
}

// DigitalInputPullUpResistor (BoolValue[8])
// Description: Decide if you want to switch on the pull-up resistor at each input pin you have chosen. (If you connect a simple switch connected to ground, you typically want this.)

// Set DigitalInputPullUpResistor (BoolValue)
// Byte offset: 200, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_digitalinputpullupresistor(uint8_t index, bool val)
{
  eeprom_write_UIntValue(200 + (uint16_t)index * 1, 0, 8, val ? 1 : 0);
}

// Get DigitalInputPullUpResistor (BoolValue)
// Byte offset: 200, bit offset: 0, length bits 8
static inline bool e2p_envsensor_get_digitalinputpullupresistor(uint8_t index)
{
  return eeprom_read_UIntValue8(200 + (uint16_t)index * 1, 0, 8, 0, 1) == 1;
}

// DigitalInputMode (EnumValue[8])
// Description: The mode decides how the device detects changes and when a new message is sent. OnChange means a status message is sent immediately after a change and only on after a larger time otherwise. Cyclic means the status is sent in the normal cycle time.

typedef enum {
  DIGITALINPUTMODE_ONCHANGE = 0,
  DIGITALINPUTMODE_CYCLIC = 1
} DigitalInputModeEnum;

// Set DigitalInputMode (EnumValue)
// Byte offset: 208, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_digitalinputmode(uint8_t index, DigitalInputModeEnum val)
{
  eeprom_write_UIntValue(208 + (uint16_t)index * 1, 0, 8, val);
}

// Get DigitalInputMode (EnumValue)
// Byte offset: 208, bit offset: 0, length bits 8
static inline DigitalInputModeEnum e2p_envsensor_get_digitalinputmode(uint8_t index)
{
  return eeprom_read_UIntValue8(208 + (uint16_t)index * 1, 0, 8, 0, 255);
}

// Reserved area with 320 bits

// AnalogInputPins (EnumValue[5])
// Description: You can choose up to 5 ADC pins as analog input. The enum values are couting through every pin from port B, C and D, leaving out the pins that are not accessible because otherwise used.

typedef enum {
  ANALOGINPUTPINS_UNUSED = 0,
  ANALOGINPUTPINS_PC1 = 10,
  ANALOGINPUTPINS_PC2 = 11,
  ANALOGINPUTPINS_PC3 = 12,
  ANALOGINPUTPINS_PC4 = 13,
  ANALOGINPUTPINS_PC5 = 14
} AnalogInputPinsEnum;

// Set AnalogInputPins (EnumValue)
// Byte offset: 256, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_analoginputpins(uint8_t index, AnalogInputPinsEnum val)
{
  eeprom_write_UIntValue(256 + (uint16_t)index * 1, 0, 8, val);
}

// Get AnalogInputPins (EnumValue)
// Byte offset: 256, bit offset: 0, length bits 8
static inline AnalogInputPinsEnum e2p_envsensor_get_analoginputpins(uint8_t index)
{
  return eeprom_read_UIntValue8(256 + (uint16_t)index * 1, 0, 8, 0, 255);
}

// AnalogInputTriggerMode (EnumValue[5])
// Description: The mode decides how the device detects changes and when a new message is sent. OnChange means a status message is sent immediately after a change and only on after a larger time otherwise. Cyclic means the status is sent in the normal cycle time.

typedef enum {
  ANALOGINPUTTRIGGERMODE_OFF = 0,
  ANALOGINPUTTRIGGERMODE_UP = 1,
  ANALOGINPUTTRIGGERMODE_DOWN = 2,
  ANALOGINPUTTRIGGERMODE_CHANGE = 3
} AnalogInputTriggerModeEnum;

// Set AnalogInputTriggerMode (EnumValue)
// Byte offset: 261, bit offset: 0, length bits 8
static inline void e2p_envsensor_set_analoginputtriggermode(uint8_t index, AnalogInputTriggerModeEnum val)
{
  eeprom_write_UIntValue(261 + (uint16_t)index * 1, 0, 8, val);
}

// Get AnalogInputTriggerMode (EnumValue)
// Byte offset: 261, bit offset: 0, length bits 8
static inline AnalogInputTriggerModeEnum e2p_envsensor_get_analoginputtriggermode(uint8_t index)
{
  return eeprom_read_UIntValue8(261 + (uint16_t)index * 1, 0, 8, 0, 255);
}

// AnalogInputTriggerThreshold (UIntValue[5])
// Description: The threshold in millivolts is used when the trigger mode is on.

// Set AnalogInputTriggerThreshold (UIntValue)
// Byte offset: 266, bit offset: 0, length bits 16, min val 0, max val 1100
static inline void e2p_envsensor_set_analoginputtriggerthreshold(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(266 + (uint16_t)index * 2, 0, 16, val);
}

// Get AnalogInputTriggerThreshold (UIntValue)
// Byte offset: 266, bit offset: 0, length bits 16, min val 0, max val 1100
static inline uint16_t e2p_envsensor_get_analoginputtriggerthreshold(uint8_t index)
{
  return eeprom_read_UIntValue16(266 + (uint16_t)index * 2, 0, 16, 0, 1100);
}

// AnalogInputTriggerHysteresis (UIntValue[5])
// Description: The hysteresis in millivolts is used when the trigger mode is on. It can avoid the trigger firing too often if you measure a slighty changing voltage. Because of noise and accuracy limits of the ADC, you should set a positive hysteresis in any case.

// Set AnalogInputTriggerHysteresis (UIntValue)
// Byte offset: 276, bit offset: 0, length bits 16, min val 0, max val 1100
static inline void e2p_envsensor_set_analoginputtriggerhysteresis(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(276 + (uint16_t)index * 2, 0, 16, val);
}

// Get AnalogInputTriggerHysteresis (UIntValue)
// Byte offset: 276, bit offset: 0, length bits 16, min val 0, max val 1100
static inline uint16_t e2p_envsensor_get_analoginputtriggerhysteresis(uint8_t index)
{
  return eeprom_read_UIntValue16(276 + (uint16_t)index * 2, 0, 16, 0, 1100);
}

// Reserved area with 272 bits

// Reserved area with 5632 bits


#endif /* _E2P_ENVSENSOR_H */
