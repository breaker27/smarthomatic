/*
* This file is part of smarthomatic, http://www.smarthomatic.com.
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
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* ! WARNING: This file is generated by the SHC EEPROM editor and should !
* ! never be modified manually.                                         !
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/


// ---------- HardwareConfig ----------

// EnumValue DeviceType

typedef enum {
  DEVICETYPE_BASE_STATION = 0,
  DEVICETYPE_TEMPERATURE_SENSOR = 20,
  DEVICETYPE_POWER_SWITCH = 40,
  DEVICETYPE_DIMMER = 60
} DeviceTypeEnum;

#define EEPROM_DEVICETYPE_BYTE 0
#define EEPROM_DEVICETYPE_BIT 0
#define EEPROM_DEVICETYPE_LENGTH_BITS 8

// UIntValue OsccalMode

#define EEPROM_OSCCALMODE_BYTE 1
#define EEPROM_OSCCALMODE_BIT 0
#define EEPROM_OSCCALMODE_LENGTH_BITS 8
#define EEPROM_OSCCALMODE_MINVAL 0
#define EEPROM_OSCCALMODE_MAXVAL 255

// Reserved area with 48 bits


// ---------- GenericConfig ----------

// UIntValue DeviceID

#define EEPROM_DEVICEID_BYTE 8
#define EEPROM_DEVICEID_BIT 0
#define EEPROM_DEVICEID_LENGTH_BITS 12
#define EEPROM_DEVICEID_MINVAL 0
#define EEPROM_DEVICEID_MAXVAL 4095

// Reserved area with 4 bits

// UIntValue PacketCounter

#define EEPROM_PACKETCOUNTER_BYTE 10
#define EEPROM_PACKETCOUNTER_BIT 0
#define EEPROM_PACKETCOUNTER_LENGTH_BITS 24
#define EEPROM_PACKETCOUNTER_MINVAL 0
#define EEPROM_PACKETCOUNTER_MAXVAL 16777215

// Reserved area with 152 bits

// ByteArray AesKey

#define EEPROM_AESKEY_BYTE 32
#define EEPROM_AESKEY_BIT 0
#define EEPROM_AESKEY_LENGTH_BYTES 32


// ---------- DimmerConfig ----------

// ByteArray BrightnessTranslationTable

#define EEPROM_BRIGHTNESSTRANSLATIONTABLE_BYTE 64
#define EEPROM_BRIGHTNESSTRANSLATIONTABLE_BIT 0
#define EEPROM_BRIGHTNESSTRANSLATIONTABLE_LENGTH_BYTES 101

// Reserved area with 6872 bits

// overall length: 8192 bits

