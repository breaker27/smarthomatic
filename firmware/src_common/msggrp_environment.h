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

#ifndef _MSGGRP_ENVIRONMENT_H
#define _MSGGRP_ENVIRONMENT_H

#include "packet_header.h"
#include "packet_headerext_common.h"
#include "packet_headerext_ackstatus.h"
#include "packet_headerext_ack.h"
#include "packet_headerext_status.h"
#include "packet_headerext_deliver.h"
#include "packet_headerext_setget.h"
#include "packet_headerext_set.h"
#include "packet_headerext_get.h"
#include "e2p_access.h"

// Message Group "environment"
// ===========================
// MessageGroupID: 11
// Description: This message group contains messages for environmental data except weather data.

// ENUM for MessageIDs of this MessageGroup
typedef enum {
  MESSAGEID_ENVIRONMENT_BRIGHTNESS = 1,
  MESSAGEID_ENVIRONMENT_DISTANCE = 2,
  MESSAGEID_ENVIRONMENT_PARTICULATEMATTER = 3
} ENVIRONMENT_MessageIDEnum;


// Message "environment_brightness"
// --------------------------------
// MessageGroupID: 11
// MessageID: 1
// Possible MessageTypes: Get, Status, AckStatus
// Validity: released
// Length w/o Header + HeaderExtension: 7 bits
// Data fields: Brightness

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_environment_brightness_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(11);
  pkg_headerext_get_set_messageid(1);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_environment_brightness_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(11);
  pkg_headerext_status_set_messageid(1);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_environment_brightness_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(11);
  pkg_headerext_ackstatus_set_messageid(1);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 10;
}

// Brightness (UIntValue)
// Description: brightness in percent

// Set Brightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 7, min val 0, max val 100
static inline void msg_environment_brightness_set_brightness(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 7, val, bufx);
}

// Get Brightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 7, min val 0, max val 100
static inline uint32_t msg_environment_brightness_get_brightness(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 7, 0, 100, bufx);
}


// Message "environment_distance"
// ------------------------------
// MessageGroupID: 11
// MessageID: 2
// Possible MessageTypes: Get, Status, AckStatus
// Validity: released
// Length w/o Header + HeaderExtension: 14 bits
// Data fields: Distance

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_environment_distance_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(11);
  pkg_headerext_get_set_messageid(2);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_environment_distance_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(11);
  pkg_headerext_status_set_messageid(2);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_environment_distance_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(11);
  pkg_headerext_ackstatus_set_messageid(2);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 10;
}

// Distance (UIntValue)
// Description: distance in cm

// Set Distance (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 14, min val 0, max val 16383
static inline void msg_environment_distance_set_distance(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 14, val, bufx);
}

// Get Distance (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 14, min val 0, max val 16383
static inline uint32_t msg_environment_distance_get_distance(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 14, 0, 16383, bufx);
}


// Message "environment_particulatematter"
// ---------------------------------------
// MessageGroupID: 11
// MessageID: 3
// Possible MessageTypes: Get, Status, AckStatus
// Validity: test
// Length w/o Header + HeaderExtension: 160 bits
// Data fields: TypicalParticleSize, Size, MassConcentration, NumberConcentration

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_environment_particulatematter_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(11);
  pkg_headerext_get_set_messageid(3);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_environment_particulatematter_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(11);
  pkg_headerext_status_set_messageid(3);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_environment_particulatematter_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(11);
  pkg_headerext_ackstatus_set_messageid(3);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 48;
  __MESSAGETYPE = 10;
}

// TypicalParticleSize (UIntValue)
// Description: Typical Particle Size [1/100 μm]. Use 1023 when value invalid.

// Set TypicalParticleSize (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 10, min val 0, max val 1023
static inline void msg_environment_particulatematter_set_typicalparticlesize(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 10, val, bufx);
}

// Get TypicalParticleSize (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 10, min val 0, max val 1023
static inline uint32_t msg_environment_particulatematter_get_typicalparticlesize(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 10, 0, 1023, bufx);
}

// Size (UIntValue[5])
// This sub-element with 8 bits is part of an element with 30 bits in a structured array.
// Description: Maximum particle size [1/10 µm] which is considered in the MassConcentration and NumberConcentration. Use 0 when the array element is not used.

// Set Size (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 30, length bits 8, min val 0, max val 255
static inline void msg_environment_particulatematter_set_size(uint8_t index, uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 30, 8, val, bufx);
}

// Get Size (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 30, length bits 8, min val 0, max val 255
static inline uint32_t msg_environment_particulatematter_get_size(uint8_t index)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 30, 8, 0, 255, bufx);
}

// MassConcentration (UIntValue[5])
// This sub-element with 10 bits is part of an element with 30 bits in a structured array.
// Description: Mass concentration [1/10 μg/m3], considering the defined size. Use 1023 when the value is invalid.

// Set MassConcentration (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 18 + (uint16_t)index * 30, length bits 10, min val 0, max val 1023
static inline void msg_environment_particulatematter_set_massconcentration(uint8_t index, uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 18 + (uint16_t)index * 30, 10, val, bufx);
}

// Get MassConcentration (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 18 + (uint16_t)index * 30, length bits 10, min val 0, max val 1023
static inline uint32_t msg_environment_particulatematter_get_massconcentration(uint8_t index)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 18 + (uint16_t)index * 30, 10, 0, 1023, bufx);
}

// NumberConcentration (UIntValue[5])
// This sub-element with 12 bits is part of an element with 30 bits in a structured array.
// Description: Number concentration [1/10 #/cm3], considering the defined size. Use 4095 when the value is invalid.

// Set NumberConcentration (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 28 + (uint16_t)index * 30, length bits 12, min val 0, max val 4095
static inline void msg_environment_particulatematter_set_numberconcentration(uint8_t index, uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 28 + (uint16_t)index * 30, 12, val, bufx);
}

// Get NumberConcentration (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 28 + (uint16_t)index * 30, length bits 12, min val 0, max val 4095
static inline uint32_t msg_environment_particulatematter_get_numberconcentration(uint8_t index)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 28 + (uint16_t)index * 30, 12, 0, 4095, bufx);
}

#endif /* _MSGGRP_ENVIRONMENT_H */
