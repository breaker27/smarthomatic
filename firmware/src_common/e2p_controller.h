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

// LCDType (EnumValue)
// Description: Define if an LCD is connected and of which type (size) it is. Only HD44780 compatible text displays are currently supported.

#ifndef _ENUM_LCDType
#define _ENUM_LCDType
typedef enum {
  LCDTYPE_NONE = 0,
  LCDTYPE_4X20 = 1,
  LCDTYPE_4X40 = 2
} LCDTypeEnum;
#endif /* _ENUM_LCDType */

// Set LCDType (EnumValue)
// Offset: 560, length bits 8
static inline void e2p_controller_set_lcdtype(LCDTypeEnum val)
{
  eeprom_write_UIntValue(560, 8, val);
}

// Get LCDType (EnumValue)
// Offset: 560, length bits 8
static inline LCDTypeEnum e2p_controller_get_lcdtype(void)
{
  return eeprom_read_UIntValue8(560, 8, 0, 255);
}

// LCDPages (UIntValue)
// Description: This is the number of additional pages that may be addressed virtually and selected with Up/Down buttons.

// Set LCDPages (UIntValue)
// Offset: 568, length bits 8, min val 0, max val 1
static inline void e2p_controller_set_lcdpages(uint8_t val)
{
  eeprom_write_UIntValue(568, 8, val);
}

// Get LCDPages (UIntValue)
// Offset: 568, length bits 8, min val 0, max val 1
static inline uint8_t e2p_controller_get_lcdpages(void)
{
  return eeprom_read_UIntValue8(568, 8, 0, 1);
}

// PageJumpBackSeconds (UIntValue)
// Description: This defines after how many seconds the display shows the first page again after the user pressed the button to show the additional virtual lines. Set 0 to disable.

// Set PageJumpBackSeconds (UIntValue)
// Offset: 576, length bits 8, min val 0, max val 255
static inline void e2p_controller_set_pagejumpbackseconds(uint8_t val)
{
  eeprom_write_UIntValue(576, 8, val);
}

// Get PageJumpBackSeconds (UIntValue)
// Offset: 576, length bits 8, min val 0, max val 255
static inline uint8_t e2p_controller_get_pagejumpbackseconds(void)
{
  return eeprom_read_UIntValue8(576, 8, 0, 255);
}

// MenuJumpBackSeconds (UIntValue)
// Description: This defines after how many seconds the selection of menu options is cancelled if the user doesn't press any button anymore. The menu options are not saved in this case. Set 0 to disable.

// Set MenuJumpBackSeconds (UIntValue)
// Offset: 584, length bits 8, min val 0, max val 255
static inline void e2p_controller_set_menujumpbackseconds(uint8_t val)
{
  eeprom_write_UIntValue(584, 8, val);
}

// Get MenuJumpBackSeconds (UIntValue)
// Offset: 584, length bits 8, min val 0, max val 255
static inline uint8_t e2p_controller_get_menujumpbackseconds(void)
{
  return eeprom_read_UIntValue8(584, 8, 0, 255);
}

// MenuOption (ByteArray[4])
// Description: These are up to 4 strings that define options that the user can select. Each entry is defined with a name and several options to select, which are separated with a pipe character '|'. Leave the string empty when there shall be no more options to select.

// Set MenuOption (ByteArray)
// Offset: 592, length bits 640
static inline void e2p_controller_set_menuoption(uint8_t index, void *src)
{
  eeprom_write_block(src, (uint8_t *)((592 + (uint16_t)index * 640) / 8), 80);
}

// Get MenuOption (ByteArray)
// Offset: 592, length bits 640
static inline void e2p_controller_get_menuoption(uint8_t index, void *dst)
{
  eeprom_read_block(dst, (uint8_t *)((592 + (uint16_t)index * 640) / 8), 80);
}

// Sound (EnumValue)
// Description: Defines if key presses and saving menu entries / cancelling the menu are acknowledged with sounds.

#ifndef _ENUM_Sound
#define _ENUM_Sound
typedef enum {
  SOUND_SILENT = 0,
  SOUND_KEY = 1,
  SOUND_SAVECANCEL = 2,
  SOUND_KEY_SAVECANCEL = 3
} SoundEnum;
#endif /* _ENUM_Sound */

// Set Sound (EnumValue)
// Offset: 3152, length bits 8
static inline void e2p_controller_set_sound(SoundEnum val)
{
  eeprom_write_UIntValue(3152, 8, val);
}

// Get Sound (EnumValue)
// Offset: 3152, length bits 8
static inline SoundEnum e2p_controller_get_sound(void)
{
  return eeprom_read_UIntValue8(3152, 8, 0, 255);
}

// OutputPinCount (UIntValue)
// Description: This is the number of output pins connected to e.g. LEDs.

// Set OutputPinCount (UIntValue)
// Offset: 3160, length bits 8, min val 1, max val 8
static inline void e2p_controller_set_outputpincount(uint8_t val)
{
  eeprom_write_UIntValue(3160, 8, val);
}

// Get OutputPinCount (UIntValue)
// Offset: 3160, length bits 8, min val 1, max val 8
static inline uint8_t e2p_controller_get_outputpincount(void)
{
  return eeprom_read_UIntValue8(3160, 8, 1, 8);
}

// Reserved area with 5024 bits
// Offset: 3168


#endif /* _E2P_CONTROLLER_H */
