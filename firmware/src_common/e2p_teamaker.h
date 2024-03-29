/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013..2019 Uwe Freese
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

#ifndef _E2P_TEAMAKER_H
#define _E2P_TEAMAKER_H

#include "e2p_access.h"

// E2P Block "TeaMaker"
// ====================
// Start offset (bit): 512
// Overall block length: 4296 bits

// BaseStationPacketCounter (UIntValue)
// Description: This is the last remembered packet counter of a command from the base station. Packets with the same or lower number are ignored.

// Set BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline void e2p_teamaker_set_basestationpacketcounter(uint32_t val)
{
  eeprom_write_UIntValue(512, 24, val);
}

// Get BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline uint32_t e2p_teamaker_get_basestationpacketcounter(void)
{
  return eeprom_read_UIntValue32(512, 24, 0, 16777215);
}

// StartupConfirmationMessage (ByteArray)
// Description: Text shown on LCD at startup until a button is pressed.

// Set StartupConfirmationMessage (ByteArray)
// Offset: 536, length bits 256
static inline void e2p_teamaker_set_startupconfirmationmessage(void *src)
{
  eeprom_write_block(src, (uint8_t *)((536) / 8), 32);
}

// Get StartupConfirmationMessage (ByteArray)
// Offset: 536, length bits 256
static inline void e2p_teamaker_get_startupconfirmationmessage(void *dst)
{
  eeprom_read_block(dst, (uint8_t *)((536) / 8), 32);
}

// Reserved area with 1280 bits
// Offset: 792

// PresetCount (UIntValue)
// Description: The number of presets to use.

// Set PresetCount (UIntValue)
// Offset: 2072, length bits 8, min val 1, max val 9
static inline void e2p_teamaker_set_presetcount(uint8_t val)
{
  eeprom_write_UIntValue(2072, 8, val);
}

// Get PresetCount (UIntValue)
// Offset: 2072, length bits 8, min val 1, max val 9
static inline uint8_t e2p_teamaker_get_presetcount(void)
{
  return eeprom_read_UIntValue8(2072, 8, 1, 9);
}

// PresetName (ByteArray[9])
// Description: The name of the brewing preset (most times the type of tea used), shown on the LCD.

// Set PresetName (ByteArray)
// Offset: 2080, length bits 128
static inline void e2p_teamaker_set_presetname(uint8_t index, void *src)
{
  eeprom_write_block(src, (uint8_t *)((2080 + (uint16_t)index * 128) / 8), 16);
}

// Get PresetName (ByteArray)
// Offset: 2080, length bits 128
static inline void e2p_teamaker_get_presetname(uint8_t index, void *dst)
{
  eeprom_read_block(dst, (uint8_t *)((2080 + (uint16_t)index * 128) / 8), 16);
}

// HeatingTemperature (UIntValue[9])
// Description: Temperature to which the water is heated up in 1/10 degrees celsius (range is 0 to 100 degree celsius).

// Set HeatingTemperature (UIntValue)
// Offset: 3232, length bits 16, min val 0, max val 1000
static inline void e2p_teamaker_set_heatingtemperature(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(3232 + (uint16_t)index * 16, 16, val);
}

// Get HeatingTemperature (UIntValue)
// Offset: 3232, length bits 16, min val 0, max val 1000
static inline uint16_t e2p_teamaker_get_heatingtemperature(uint8_t index)
{
  return eeprom_read_UIntValue16(3232 + (uint16_t)index * 16, 16, 0, 1000);
}

// HeatingTemperatureDrop (UIntValue[9])
// Description: The number of degrees that the temperature has to drop after the maximum temperature was reached after powering off the heating plate when HeatingTemperature was reached (in 1/10 degree celsius).

// Set HeatingTemperatureDrop (UIntValue)
// Offset: 3376, length bits 8, min val 0, max val 50
static inline void e2p_teamaker_set_heatingtemperaturedrop(uint8_t index, uint8_t val)
{
  eeprom_write_UIntValue(3376 + (uint16_t)index * 8, 8, val);
}

// Get HeatingTemperatureDrop (UIntValue)
// Offset: 3376, length bits 8, min val 0, max val 50
static inline uint8_t e2p_teamaker_get_heatingtemperaturedrop(uint8_t index)
{
  return eeprom_read_UIntValue8(3376 + (uint16_t)index * 8, 8, 0, 50);
}

// LastHeatingTimeSec (UIntValue[9])
// Description: This is the time it took the last time to heat up the water. The value is used to show and calculate a countdown. Range is from 0 to 30 minutes, in seconds.

// Set LastHeatingTimeSec (UIntValue)
// Offset: 3448, length bits 16, min val 0, max val 1800
static inline void e2p_teamaker_set_lastheatingtimesec(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(3448 + (uint16_t)index * 16, 16, val);
}

// Get LastHeatingTimeSec (UIntValue)
// Offset: 3448, length bits 16, min val 0, max val 1800
static inline uint16_t e2p_teamaker_get_lastheatingtimesec(uint8_t index)
{
  return eeprom_read_UIntValue16(3448 + (uint16_t)index * 16, 16, 0, 1800);
}

// BrewingTemperature (UIntValue[9])
// Description: Temperature (in 1/10 degree celsius) to which the water is cooled off until brewing is started, in 1/10 degree celsius. The water is held at this temperature during the brewing phase.

// Set BrewingTemperature (UIntValue)
// Offset: 3592, length bits 16, min val 0, max val 1000
static inline void e2p_teamaker_set_brewingtemperature(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(3592 + (uint16_t)index * 16, 16, val);
}

// Get BrewingTemperature (UIntValue)
// Offset: 3592, length bits 16, min val 0, max val 1000
static inline uint16_t e2p_teamaker_get_brewingtemperature(uint8_t index)
{
  return eeprom_read_UIntValue16(3592 + (uint16_t)index * 16, 16, 0, 1000);
}

// BrewingTimeSec (UIntValue[9])
// Description: This is how long the tea bag is placed inside the water at brewing temperature. Range is from 0 to 30 minutes, in seconds.

// Set BrewingTimeSec (UIntValue)
// Offset: 3736, length bits 16, min val 0, max val 1800
static inline void e2p_teamaker_set_brewingtimesec(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(3736 + (uint16_t)index * 16, 16, val);
}

// Get BrewingTimeSec (UIntValue)
// Offset: 3736, length bits 16, min val 0, max val 1800
static inline uint16_t e2p_teamaker_get_brewingtimesec(uint8_t index)
{
  return eeprom_read_UIntValue16(3736 + (uint16_t)index * 16, 16, 0, 1800);
}

// WarmingTemperature (UIntValue[9])
// Description: Temperature at which the water is held after brewing is finished.

// Set WarmingTemperature (UIntValue)
// Offset: 3880, length bits 16, min val 0, max val 1000
static inline void e2p_teamaker_set_warmingtemperature(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(3880 + (uint16_t)index * 16, 16, val);
}

// Get WarmingTemperature (UIntValue)
// Offset: 3880, length bits 16, min val 0, max val 1000
static inline uint16_t e2p_teamaker_get_warmingtemperature(uint8_t index)
{
  return eeprom_read_UIntValue16(3880 + (uint16_t)index * 16, 16, 0, 1000);
}

// WarmingTimeSec (UIntValue[9])
// Description: This is how long the water is held at the warming temperature after brewing is finished. Range is from 0 to 12 hours, in seconds.

// Set WarmingTimeSec (UIntValue)
// Offset: 4024, length bits 16, min val 0, max val 43200
static inline void e2p_teamaker_set_warmingtimesec(uint8_t index, uint16_t val)
{
  eeprom_write_UIntValue(4024 + (uint16_t)index * 16, 16, val);
}

// Get WarmingTimeSec (UIntValue)
// Offset: 4024, length bits 16, min val 0, max val 43200
static inline uint16_t e2p_teamaker_get_warmingtimesec(uint8_t index)
{
  return eeprom_read_UIntValue16(4024 + (uint16_t)index * 16, 16, 0, 43200);
}

// Reserved area with 576 bits
// Offset: 4168

// BrewingRegulationRangeAbove (UIntValue)
// Description: This defines at which temperature the heating plate is turned off when heating up in the brewing phase, in 1/10 degree celsius.

// Set BrewingRegulationRangeAbove (UIntValue)
// Offset: 4744, length bits 8, min val 0, max val 50
static inline void e2p_teamaker_set_brewingregulationrangeabove(uint8_t val)
{
  eeprom_write_UIntValue(4744, 8, val);
}

// Get BrewingRegulationRangeAbove (UIntValue)
// Offset: 4744, length bits 8, min val 0, max val 50
static inline uint8_t e2p_teamaker_get_brewingregulationrangeabove(void)
{
  return eeprom_read_UIntValue8(4744, 8, 0, 50);
}

// BrewingRegulationRangeBelow (UIntValue)
// Description: This defines at which temperature the heating plate is turned on again when the water is cooling off currently in the brewing phase, in 1/10 degree celsius. 

// Set BrewingRegulationRangeBelow (UIntValue)
// Offset: 4752, length bits 8, min val 0, max val 50
static inline void e2p_teamaker_set_brewingregulationrangebelow(uint8_t val)
{
  eeprom_write_UIntValue(4752, 8, val);
}

// Get BrewingRegulationRangeBelow (UIntValue)
// Offset: 4752, length bits 8, min val 0, max val 50
static inline uint8_t e2p_teamaker_get_brewingregulationrangebelow(void)
{
  return eeprom_read_UIntValue8(4752, 8, 0, 50);
}

// WarmingRegulationRangeAbove (UIntValue)
// Description: This defines at which temperature the heating plate is turned off when heating up in the warming phase, in 1/10 degree celsius.

// Set WarmingRegulationRangeAbove (UIntValue)
// Offset: 4760, length bits 8, min val 0, max val 50
static inline void e2p_teamaker_set_warmingregulationrangeabove(uint8_t val)
{
  eeprom_write_UIntValue(4760, 8, val);
}

// Get WarmingRegulationRangeAbove (UIntValue)
// Offset: 4760, length bits 8, min val 0, max val 50
static inline uint8_t e2p_teamaker_get_warmingregulationrangeabove(void)
{
  return eeprom_read_UIntValue8(4760, 8, 0, 50);
}

// WarmingRegulationRangeBelow (UIntValue)
// Description: This defines at which temperature the heating plate is turned on again when the water is cooling off currently in the warming phase, in 1/10 degree celsius.

// Set WarmingRegulationRangeBelow (UIntValue)
// Offset: 4768, length bits 8, min val 0, max val 50
static inline void e2p_teamaker_set_warmingregulationrangebelow(uint8_t val)
{
  eeprom_write_UIntValue(4768, 8, val);
}

// Get WarmingRegulationRangeBelow (UIntValue)
// Offset: 4768, length bits 8, min val 0, max val 50
static inline uint8_t e2p_teamaker_get_warmingregulationrangebelow(void)
{
  return eeprom_read_UIntValue8(4768, 8, 0, 50);
}

// BrewingPWMPercentage (UIntValue)
// Description: The percentage of time the heating plate is one in one cycle of several seconds to minutes. The value has to be high enough to actually heat up the water also at the maximum brewing temperature and the maximum amount of water in the kettle.

// Set BrewingPWMPercentage (UIntValue)
// Offset: 4776, length bits 8, min val 0, max val 100
static inline void e2p_teamaker_set_brewingpwmpercentage(uint8_t val)
{
  eeprom_write_UIntValue(4776, 8, val);
}

// Get BrewingPWMPercentage (UIntValue)
// Offset: 4776, length bits 8, min val 0, max val 100
static inline uint8_t e2p_teamaker_get_brewingpwmpercentage(void)
{
  return eeprom_read_UIntValue8(4776, 8, 0, 100);
}

// BrewingPWMCycleSec (UIntValue)
// Description: The length of one heating PWM cycle in seconds in the brewing phase.

// Set BrewingPWMCycleSec (UIntValue)
// Offset: 4784, length bits 8, min val 0, max val 255
static inline void e2p_teamaker_set_brewingpwmcyclesec(uint8_t val)
{
  eeprom_write_UIntValue(4784, 8, val);
}

// Get BrewingPWMCycleSec (UIntValue)
// Offset: 4784, length bits 8, min val 0, max val 255
static inline uint8_t e2p_teamaker_get_brewingpwmcyclesec(void)
{
  return eeprom_read_UIntValue8(4784, 8, 0, 255);
}

// WarmingPWMPercentage (UIntValue)
// Description: The percentage of time the heating plate is one in one cycle of several seconds to minutes. The value has to be high enough to actually heat up the water also at the maximum warming temperature and the maximum amount of water in the kettle. The PWM cycle in general should be slower in the warming phase compared to the brewing phase, because usually the temperature to regulate to is not so important.

// Set WarmingPWMPercentage (UIntValue)
// Offset: 4792, length bits 8, min val 0, max val 100
static inline void e2p_teamaker_set_warmingpwmpercentage(uint8_t val)
{
  eeprom_write_UIntValue(4792, 8, val);
}

// Get WarmingPWMPercentage (UIntValue)
// Offset: 4792, length bits 8, min val 0, max val 100
static inline uint8_t e2p_teamaker_get_warmingpwmpercentage(void)
{
  return eeprom_read_UIntValue8(4792, 8, 0, 100);
}

// WarmingPWMCycleSec (UIntValue)
// Description: The length of one heating PWM cycle in seconds in the warming phase.

// Set WarmingPWMCycleSec (UIntValue)
// Offset: 4800, length bits 8, min val 0, max val 255
static inline void e2p_teamaker_set_warmingpwmcyclesec(uint8_t val)
{
  eeprom_write_UIntValue(4800, 8, val);
}

// Get WarmingPWMCycleSec (UIntValue)
// Offset: 4800, length bits 8, min val 0, max val 255
static inline uint8_t e2p_teamaker_get_warmingpwmcyclesec(void)
{
  return eeprom_read_UIntValue8(4800, 8, 0, 255);
}


#endif /* _E2P_TEAMAKER_H */
