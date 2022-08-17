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

#ifndef _PACKET_HEADEREXT_STATUS_H
#define _PACKET_HEADEREXT_STATUS_H

#include "packet_header.h"

// MessageGroupID (UIntValue)
// Description: 0 = Generic, 1..9 = Reserved, others are arbitrary

// Set MessageGroupID (UIntValue)
// Offset: 72, length bits 7, min val 0, max val 127
static inline void pkg_headerext_status_set_messagegroupid(uint32_t val)
{
  array_write_UIntValue(72, 7, val, bufx);
}

// Get MessageGroupID (UIntValue)
// Offset: 72, length bits 7, min val 0, max val 127
static inline uint32_t pkg_headerext_status_get_messagegroupid(void)
{
  return array_read_UIntValue32(72, 7, 0, 127, bufx);
}

// MessageID (UIntValue)

// Set MessageID (UIntValue)
// Offset: 79, length bits 4, min val 0, max val 15
static inline void pkg_headerext_status_set_messageid(uint32_t val)
{
  array_write_UIntValue(79, 4, val, bufx);
}

// Get MessageID (UIntValue)
// Offset: 79, length bits 4, min val 0, max val 15
static inline uint32_t pkg_headerext_status_get_messageid(void)
{
  return array_read_UIntValue32(79, 4, 0, 15, bufx);
}


// overall length: 83 bits
// message data follows: yes

#endif /* _PACKET_HEADEREXT_STATUS_H */
