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

#ifndef _E2P_CONTROLLER_H
#define _E2P_CONTROLLER_H

#include "e2p_access.h"

// E2P Block "Controller"
// ======================
// Start offset (bit): 512
// Overall block length: 7680 bits

// BaseStationPacketCounter (UIntValue)
// Description: This is the last remembered packet counter of a command from the base station. Packets with the same or lower number are ignored.

// Set BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline void e2p_controller_set_basestationpacketcounter(uint32_t val)
{
  eeprom_write_UIntValue(512, 24, val);
}

// Get BaseStationPacketCounter (UIntValue)
// Offset: 512, length bits 24, min val 0, max val 16777215
static inline uint32_t e2p_controller_get_basestationpacketcounter(void)
{
  return eeprom_read_UIntValue32(512, 24, 0, 16777215);
}

// BrightnessFactor (UIntValue)
// Description: This value reduces the overall brightness. This is to easily adjust it to your needs without changing the LED series resistors. WARNING: It is recommended to use this only for testing, because it reduces the amount of different brightness levels. Changing the resistors is to be preferred.

// Set BrightnessFactor (UIntValue)
// Offset: 536, length bits 8, min val 1, max val 100
static inline void e2p_controller_set_brightnessfactor(uint8_t val)
{
  eeprom_write_UIntValue(536, 8, val);
}

// Get BrightnessFactor (UIntValue)
// Offset: 536, length bits 8, min val 1, max val 100
static inline uint8_t e2p_controller_get_brightnessfactor(void)
{
  return eeprom_read_UIntValue8(536, 8, 1, 100);
}

// TransceiverWatchdogTimeout (UIntValue)
// Description: Reset RFM12B module if no data is received until timeout is reached. Use this function if your specific transceiver hangs sometimes. Value is in deca seconds. Suggested setting is 48 (for 8 minutes). Set 0 to disable.

// Set TransceiverWatchdogTimeout (UIntValue)
// Offset: 544, length bits 8, min val 0, max val 255
static inline void e2p_controller_set_transceiverwatchdogtimeout(uint8_t val)
{
  eeprom_write_UIntValue(544, 8, val);
}

// Get TransceiverWatchdogTimeout (UIntValue)
// Offset: 544, length bits 8, min val 0, max val 255
static inline uint8_t e2p_controller_get_transceiverwatchdogtimeout(void)
{
  return eeprom_read_UIntValue8(544, 8, 0, 255);
}

// StatusCycle (UIntValue)
// Description: This is the number of minutes after which the status should be resent, so in case of a lost message it can be received again. Set 0 to disable.

// Set StatusCycle (UIntValue)
// Offset: 552, length bits 8, min val 0, max val 255
static inline void e2p_controller_set_statuscycle(uint8_t val)
{
  eeprom_write_UIntValue(552, 8, val);
}

// Get StatusCycle (UIntValue)
// Offset: 552, length bits 8, min val 0, max val 255
static inline uint8_t e2p_controller_get_statuscycle(void)
{
  return eeprom_read_UIntValue8(552, 8, 0, 255);
}

// InputPinCount (UIntValue)
// Description: This is the number of input pins connected to e.g. buttons or switches.

// Set InputPinCount (UIntValue)
// Offset: 560, length bits 8, min val 1, max val 8
static inline void e2p_controller_set_inputpincount(uint8_t val)
{
  eeprom_write_UIntValue(560, 8, val);
}

// Get InputPinCount (UIntValue)
// Offset: 560, length bits 8, min val 1, max val 8
static inline uint8_t e2p_controller_get_inputpincount(void)
{
  return eeprom_read_UIntValue8(560, 8, 1, 8);
}

// OutputPinCount (UIntValue)
// Description: This is the number of output pins connected to e.g. LEDs.

// Set OutputPinCount (UIntValue)
// Offset: 568, length bits 8, min val 1, max val 8
static inline void e2p_controller_set_outputpincount(uint8_t val)
{
  eeprom_write_UIntValue(568, 8, val);
}

// Get OutputPinCount (UIntValue)
// Offset: 568, length bits 8, min val 1, max val 8
static inline uint8_t e2p_controller_get_outputpincount(void)
{
  return eeprom_read_UIntValue8(568, 8, 1, 8);
}

// Width (UIntValue)
// Description: This is the number of characters per line on the LCD. Typical values are 8, 16, 20, 40.

// Set Width (UIntValue)
// Offset: 576, length bits 8, min val 1, max val 40
static inline void e2p_controller_set_width(uint8_t val)
{
  eeprom_write_UIntValue(576, 8, val);
}

// Get Width (UIntValue)
// Offset: 576, length bits 8, min val 1, max val 40
static inline uint8_t e2p_controller_get_width(void)
{
  return eeprom_read_UIntValue8(576, 8, 1, 40);
}

// Height (UIntValue)
// Description: This is the number of lines on the LCD. Typical values are 1, 2 and 4.

// Set Height (UIntValue)
// Offset: 584, length bits 8, min val 1, max val 4
static inline void e2p_controller_set_height(uint8_t val)
{
  eeprom_write_UIntValue(584, 8, val);
}

// Get Height (UIntValue)
// Offset: 584, length bits 8, min val 1, max val 4
static inline uint8_t e2p_controller_get_height(void)
{
  return eeprom_read_UIntValue8(584, 8, 1, 4);
}

// HeightVirtual (UIntValue)
// Description: This is the number of additional lines that may be addressed virtually. If the number is heigher than 'Height', the pages can be cycled through with button 9 (which is not part of the buttons from the InputPinCount, which are being sent as status).

// Set HeightVirtual (UIntValue)
// Offset: 592, length bits 8, min val 1, max val 4
static inline void e2p_controller_set_heightvirtual(uint8_t val)
{
  eeprom_write_UIntValue(592, 8, val);
}

// Get HeightVirtual (UIntValue)
// Offset: 592, length bits 8, min val 1, max val 4
static inline uint8_t e2p_controller_get_heightvirtual(void)
{
  return eeprom_read_UIntValue8(592, 8, 1, 4);
}

// AutoJumpBackSeconds (UIntValue)
// Description: This defines after how many seconds the display shows the first lines again after the user pressed the button to show the additional virtual lines. Set 0 to disable.

// Set AutoJumpBackSeconds (UIntValue)
// Offset: 600, length bits 8, min val 0, max val 255
static inline void e2p_controller_set_autojumpbackseconds(uint8_t val)
{
  eeprom_write_UIntValue(600, 8, val);
}

// Get AutoJumpBackSeconds (UIntValue)
// Offset: 600, length bits 8, min val 0, max val 255
static inline uint8_t e2p_controller_get_autojumpbackseconds(void)
{
  return eeprom_read_UIntValue8(600, 8, 0, 255);
}

// Reserved area with 7584 bits
// Offset: 608


#endif /* _E2P_CONTROLLER_H */
