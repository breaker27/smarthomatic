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

#ifndef _PACKET_HEADER_H
#define _PACKET_HEADER_H

#include <stdbool.h>
#include "util.h"
#include "e2p_access.h"

// Header size in bits (incl. header extension), set depending on used
// MessageType and used for calculating message data offsets.
uint8_t __HEADEROFFSETBITS;

// Packet size in bytes including padding, set depending on MessageType,
// MessageGroupID and MessageID and used for CRC32 calculation.
uint8_t __PACKETSIZEBYTES;

// Remember the MessageType after receiving a packet to reduce code size
// in common header extension access functions.
uint8_t __MESSAGETYPE;

// ENUM MessageGroupID
typedef enum {
  MESSAGEGROUP_GENERIC = 0,
  MESSAGEGROUP_GPIO = 1,
  MESSAGEGROUP_WEATHER = 10,
  MESSAGEGROUP_ENVIRONMENT = 11,
  MESSAGEGROUP_POWERSWITCH = 20,
  MESSAGEGROUP_DIMMER = 60
} MessageGroupIDEnum;

// CRC32 (UIntValue)
// Description: CRC32 value of byte 4..n

// Set CRC32 (UIntValue)
// Offset: 0, length bits 32, min val 0, max val 4294967295
static inline void pkg_header_set_crc32(uint32_t val)
{
  array_write_UIntValue(0, 32, val, bufx);
}

// Get CRC32 (UIntValue)
// Offset: 0, length bits 32, min val 0, max val 4294967295
static inline uint32_t pkg_header_get_crc32(void)
{
  return array_read_UIntValue32(0, 32, 0, 4294967295U, bufx);
}

// SenderID (UIntValue)
// Description: The SenderID is the DeviceID of the sending device. It's only allowed to send packets with the own DeviceID. 0 = base station, others are arbitrary.

// Set SenderID (UIntValue)
// Offset: 32, length bits 12, min val 0, max val 4095
static inline void pkg_header_set_senderid(uint32_t val)
{
  array_write_UIntValue(32, 12, val, bufx);
}

// Get SenderID (UIntValue)
// Offset: 32, length bits 12, min val 0, max val 4095
static inline uint32_t pkg_header_get_senderid(void)
{
  return array_read_UIntValue32(32, 12, 0, 4095, bufx);
}

// PacketCounter (UIntValue)
// Description: The PacketCounter is counted up throughout the whole lifetime of the device and is used to make the encrypted packets differently from each other every time. Packets received with the same or lower number must be ignored per SenderID.

// Set PacketCounter (UIntValue)
// Offset: 44, length bits 24, min val 0, max val 16777215
static inline void pkg_header_set_packetcounter(uint32_t val)
{
  array_write_UIntValue(44, 24, val, bufx);
}

// Get PacketCounter (UIntValue)
// Offset: 44, length bits 24, min val 0, max val 16777215
static inline uint32_t pkg_header_get_packetcounter(void)
{
  return array_read_UIntValue32(44, 24, 0, 16777215, bufx);
}

// MessageType (EnumValue)
// Description: The message type influences the behaviour with sending the packet. E.g. requests are acknowledged.

typedef enum {
  MESSAGETYPE_GET = 0,
  MESSAGETYPE_SET = 1,
  MESSAGETYPE_SETGET = 2,
  MESSAGETYPE_STATUS = 8,
  MESSAGETYPE_ACK = 9,
  MESSAGETYPE_ACKSTATUS = 10
} MessageTypeEnum;

// Set MessageType (EnumValue)
// Offset: 68, length bits 4
static inline void pkg_header_set_messagetype(MessageTypeEnum val)
{
  array_write_UIntValue(68, 4, val, bufx);
}

// Get MessageType (EnumValue)
// Offset: 68, length bits 4
static inline MessageTypeEnum pkg_header_get_messagetype(void)
{
  return array_read_UIntValue32(68, 4, 0, 15, bufx);
}


// overall length: 72 bits

// Function to set CRC value after all data fields are set.
static inline void pkg_header_calc_crc32(void)
{
  pkg_header_set_crc32(crc32(bufx + 4, __PACKETSIZEBYTES - 4));
}

// Function to check CRC value against calculated one (after reception).
static inline bool pkg_header_check_crc32(uint8_t packet_size_bytes)
{
  return getBuf32(0) == crc32(bufx + 4, packet_size_bytes - 4);
}

#endif /* _PACKET_HEADER_H */
