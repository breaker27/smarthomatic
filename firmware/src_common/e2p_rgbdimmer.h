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

#ifndef _E2P_RGBDIMMER_H
#define _E2P_RGBDIMMER_H

#include "e2p_access.h"

// E2P Block "RGBDimmer"
// =====================
// Start offset (bit): 512
// Overall block length: 7688 bits

// BaseStationPacketCounter (UIntValue)
// Description: This is the last remembered packet counter of a command from the base station. Packets with the same or lower number are ignored.

// Set BaseStationPacketCounter (UIntValue)
// Byte offset: 64, bit offset: 0, length bits 24, min val 0, max val 16777215
static inline void e2p_rgbdimmer_set_basestationpacketcounter(uint32_t val)
{
  eeprom_write_UIntValue(64, 0, 24, val);
}

// Get BaseStationPacketCounter (UIntValue)
// Byte offset: 64, bit offset: 0, length bits 24, min val 0, max val 16777215
static inline uint32_t e2p_rgbdimmer_get_basestationpacketcounter(void)
{
  return eeprom_read_UIntValue32(64, 0, 24, 0, 16777215);
}

// ColorMixMode (EnumValue)
// Description: This value decides how the colors are mixed together. 'Balanced' reduces the power of the all LEDs when you select a color where more than one LED is on. It reduces the maximum power consumption and needed cooling. Additive does not do that. E.g. 255,255,255 white is than brighter than 255 red.

typedef enum {
  COLORMIXMODE_BALANCED = 0,
  COLORMIXMODE_ADDITIVE = 1
} ColorMixModeEnum;

// Set ColorMixMode (EnumValue)
// Byte offset: 67, bit offset: 0, length bits 8
static inline void e2p_rgbdimmer_set_colormixmode(ColorMixModeEnum val)
{
  eeprom_write_UIntValue(67, 0, 8, val);
}

// Get ColorMixMode (EnumValue)
// Byte offset: 67, bit offset: 0, length bits 8
static inline ColorMixModeEnum e2p_rgbdimmer_get_colormixmode(void)
{
  return eeprom_read_UIntValue8(67, 0, 8, 0, 255);
}

// BrightnessFactor (UIntValue)
// Description: This value reduces the overall brightness. This is to easily adjust it to your needs without changing the LED series resistors.

// Set BrightnessFactor (UIntValue)
// Byte offset: 68, bit offset: 0, length bits 8, min val 1, max val 100
static inline void e2p_rgbdimmer_set_brightnessfactor(uint8_t val)
{
  eeprom_write_UIntValue(68, 0, 8, val);
}

// Get BrightnessFactor (UIntValue)
// Byte offset: 68, bit offset: 0, length bits 8, min val 1, max val 100
static inline uint8_t e2p_rgbdimmer_get_brightnessfactor(void)
{
  return eeprom_read_UIntValue8(68, 0, 8, 1, 100);
}

// BrightnessTranslationTableR (ByteArray)
// Description: These are the target values (one byte each) for the input brightness of 0, 1, ... 100% to adapt the specific brightness curve of your *red* LED. Set first byte to FF to not use it.

// Set BrightnessTranslationTableR (ByteArray)
// Byte offset: 69, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_set_brightnesstranslationtabler(void *src)
{
  eeprom_write_block(src, (uint8_t *)(69), 101);
}

// Get BrightnessTranslationTableR (ByteArray)
// Byte offset: 69, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_get_brightnesstranslationtabler(void *dst)
{
  eeprom_read_block(dst, (uint8_t *)(69), 101);
}

// BrightnessTranslationTableG (ByteArray)
// Description: These are the target values (one byte each) for the input brightness of 0, 1, ... 100% to adapt the specific brightness curve of your *green* LED. Set first byte to FF to not use it.

// Set BrightnessTranslationTableG (ByteArray)
// Byte offset: 170, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_set_brightnesstranslationtableg(void *src)
{
  eeprom_write_block(src, (uint8_t *)(170), 101);
}

// Get BrightnessTranslationTableG (ByteArray)
// Byte offset: 170, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_get_brightnesstranslationtableg(void *dst)
{
  eeprom_read_block(dst, (uint8_t *)(170), 101);
}

// BrightnessTranslationTableB (ByteArray)
// Description: These are the target values (one byte each) for the input brightness of 0, 1, ... 100% to adapt the specific brightness curve of your *blue* LED. Set first byte to FF to not use it.

// Set BrightnessTranslationTableB (ByteArray)
// Byte offset: 271, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_set_brightnesstranslationtableb(void *src)
{
  eeprom_write_block(src, (uint8_t *)(271), 101);
}

// Get BrightnessTranslationTableB (ByteArray)
// Byte offset: 271, bit offset: 0, length bits 808
static inline void e2p_rgbdimmer_get_brightnesstranslationtableb(void *dst)
{
  eeprom_read_block(dst, (uint8_t *)(271), 101);
}

// Reserved area with 5224 bits


#endif /* _E2P_RGBDIMMER_H */
