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

#ifndef _E2P_RGBDIMMER_H
#define _E2P_RGBDIMMER_H

#include "e2p_access.h"

// E2P Block "RGBDimmer"
// =====================
// Start offset (bit): 512
// Overall block length: 7680 bits

// BaseStationPacketCounter (UIntValue)
// Description: This is the last remembered packet counter of a command from the base station. Packets with the same or lower number are ignored.

// Set BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline void e2p_rgbdimmer_set_basestationpacketcounter(uint32_t val)
{
  eeprom_write_UIntValue(512, 24, val);
}

// Get BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline uint32_t e2p_rgbdimmer_get_basestationpacketcounter(void)
{
  return eeprom_read_UIntValue32(512, 24, 0, 16777215);
}

// BrightnessFactor (UIntValue)
// Description: This value reduces the overall brightness. This is to easily adjust it to your needs without changing the LED series resistors. WARNING: It is recommended to use this only for testing, because it reduces the amount of different brightness levels. Changing the resistors is to be preferred.

// Set BrightnessFactor (UIntValue)
// Offset: 536, length bits 8, min val 1, max val 100
static inline void e2p_rgbdimmer_set_brightnessfactor(uint8_t val)
{
  eeprom_write_UIntValue(536, 8, val);
}

// Get BrightnessFactor (UIntValue)
// Offset: 536, length bits 8, min val 1, max val 100
static inline uint8_t e2p_rgbdimmer_get_brightnessfactor(void)
{
  return eeprom_read_UIntValue8(536, 8, 1, 100);
}

// TransceiverWatchdogTimeout (UIntValue)
// Description: Reset RFM12B module if no data is received until timeout is reached. Use this function if your specific transceiver hangs sometimes. Value is in deca seconds. Suggested setting is 48 (for 8 minutes). Set 0 to disable.

// Set TransceiverWatchdogTimeout (UIntValue)
// Offset: 544, length bits 8, min val 0, max val 255
static inline void e2p_rgbdimmer_set_transceiverwatchdogtimeout(uint8_t val)
{
  eeprom_write_UIntValue(544, 8, val);
}

// Get TransceiverWatchdogTimeout (UIntValue)
// Offset: 544, length bits 8, min val 0, max val 255
static inline uint8_t e2p_rgbdimmer_get_transceiverwatchdogtimeout(void)
{
  return eeprom_read_UIntValue8(544, 8, 0, 255);
}

// Reserved area with 7640 bits
// Offset: 552


#endif /* _E2P_RGBDIMMER_H */
