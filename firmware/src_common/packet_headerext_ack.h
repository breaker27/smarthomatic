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

#ifndef _PACKET_HEADEREXT_ACK_H
#define _PACKET_HEADEREXT_ACK_H

#include "packet_header.h"

// AckSenderID (UIntValue)
// Description: The ID of the requestor whose request is acknowledged.

// Set AckSenderID (UIntValue)
// Offset: 72, length bits 12, min val 0, max val 4095
static inline void pkg_headerext_ack_set_acksenderid(uint32_t val)
{
  array_write_UIntValue(72, 12, val, bufx);
}

// Get AckSenderID (UIntValue)
// Offset: 72, length bits 12, min val 0, max val 4095
static inline uint32_t pkg_headerext_ack_get_acksenderid(void)
{
  return array_read_UIntValue32(72, 12, 0, 4095, bufx);
}

// AckPacketCounter (UIntValue)
// Description: The PacketCounter of the request that is acknowledged.

// Set AckPacketCounter (UIntValue)
// Offset: 84, length bits 24, min val 0, max val 16777215
static inline void pkg_headerext_ack_set_ackpacketcounter(uint32_t val)
{
  array_write_UIntValue(84, 24, val, bufx);
}

// Get AckPacketCounter (UIntValue)
// Offset: 84, length bits 24, min val 0, max val 16777215
static inline uint32_t pkg_headerext_ack_get_ackpacketcounter(void)
{
  return array_read_UIntValue32(84, 24, 0, 16777215, bufx);
}

// Error (BoolValue)
// Description: Tells if there was an error fulfilling the request or not.

// Set Error (BoolValue)
// Offset: 108, length bits 1
static inline void pkg_headerext_ack_set_error(bool val)
{
  array_write_UIntValue(108, 1, val ? 1 : 0, bufx);
}

// Get Error (BoolValue)
// Offset: 108, length bits 1
static inline bool pkg_headerext_ack_get_error(void)
{
  return array_read_UIntValue8(108, 1, 0, 1, bufx) == 1;
}


// overall length: 109 bits
// message data follows: no

#endif /* _PACKET_HEADEREXT_ACK_H */
