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

#ifndef _MSGGRP_DIMMER_H
#define _MSGGRP_DIMMER_H

#include "packet_header.h"
#include "packet_headerext_common.h"
#include "packet_headerext_ackstatus.h"
#include "packet_headerext_ack.h"
#include "packet_headerext_status.h"
#include "packet_headerext_setget.h"
#include "packet_headerext_set.h"
#include "packet_headerext_get.h"
#include "e2p_access.h"

// Message Group "dimmer"
// ======================
// MessageGroupID: 60

// ENUM for MessageIDs of this MessageGroup
typedef enum {
  MESSAGEID_DIMMER_BRIGHTNESS = 1,
  MESSAGEID_DIMMER_ANIMATION = 2,
  MESSAGEID_DIMMER_COLOR = 10,
  MESSAGEID_DIMMER_COLORANIMATION = 11
} DIMMER_MessageIDEnum;


// Message "dimmer_brightness"
// ---------------------------
// MessageGroupID: 60
// MessageID: 1
// Possible MessageTypes: Get, Set, SetGet, Status, Ack, AckStatus
// Validity: test
// Length w/o Header + HeaderExtension: 7 bits
// Data fields: Brightness
// Description: This is to set a fixed brightness.

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_dimmer_brightness_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(60);
  pkg_headerext_get_set_messageid(1);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Set".
static inline void pkg_header_init_dimmer_brightness_set(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(1);
  pkg_headerext_set_set_messagegroupid(60);
  pkg_headerext_set_set_messageid(1);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 1;
}

// Function to initialize header for the MessageType "SetGet".
static inline void pkg_header_init_dimmer_brightness_setget(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(2);
  pkg_headerext_setget_set_messagegroupid(60);
  pkg_headerext_setget_set_messageid(1);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 2;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_dimmer_brightness_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(60);
  pkg_headerext_status_set_messageid(1);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "Ack".
static inline void pkg_header_init_dimmer_brightness_ack(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(9);
  __HEADEROFFSETBITS = 109;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 9;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_dimmer_brightness_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(60);
  pkg_headerext_ackstatus_set_messageid(1);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 10;
}

// Brightness (UIntValue)
// Description: The brightness in percent. 0 = Off.

// Set Brightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 7, min val 0, max val 100
static inline void msg_dimmer_brightness_set_brightness(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 7, val, bufx);
}

// Get Brightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 7, min val 0, max val 100
static inline uint32_t msg_dimmer_brightness_get_brightness(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 7, 0, 100, bufx);
}


// Message "dimmer_animation"
// --------------------------
// MessageGroupID: 60
// MessageID: 2
// Possible MessageTypes: Get, Set, SetGet, Status, Ack, AckStatus
// Validity: test
// Length w/o Header + HeaderExtension: 32 bits
// Data fields: AnimationMode, TimeoutSec, StartBrightness, EndBrightness
// Description: This is the state of the dimmer output voltage and its timeout value.

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_dimmer_animation_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(60);
  pkg_headerext_get_set_messageid(2);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Set".
static inline void pkg_header_init_dimmer_animation_set(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(1);
  pkg_headerext_set_set_messagegroupid(60);
  pkg_headerext_set_set_messageid(2);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 1;
}

// Function to initialize header for the MessageType "SetGet".
static inline void pkg_header_init_dimmer_animation_setget(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(2);
  pkg_headerext_setget_set_messagegroupid(60);
  pkg_headerext_setget_set_messageid(2);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 2;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_dimmer_animation_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(60);
  pkg_headerext_status_set_messageid(2);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "Ack".
static inline void pkg_header_init_dimmer_animation_ack(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(9);
  __HEADEROFFSETBITS = 109;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 9;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_dimmer_animation_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(60);
  pkg_headerext_ackstatus_set_messageid(2);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 10;
}

// AnimationMode (EnumValue)
// Description: If a time is set, use this animation mode to change the brightness over time (none = leave at start state for the whole time and switch to end state at the end).

#ifndef _ENUM_AnimationMode
#define _ENUM_AnimationMode
typedef enum {
  ANIMATIONMODE_NONE = 0,
  ANIMATIONMODE_LINEAR = 1
} AnimationModeEnum;
#endif /* _ENUM_AnimationMode */

// Set AnimationMode (EnumValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 2
static inline void msg_dimmer_animation_set_animationmode(AnimationModeEnum val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 2, val, bufx);
}

// Get AnimationMode (EnumValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 2
static inline AnimationModeEnum msg_dimmer_animation_get_animationmode(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 2, 0, 3, bufx);
}

// TimeoutSec (UIntValue)
// Description: The time for the animation. Use 0 to disable this.

// Set TimeoutSec (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 2, length bits 16, min val 0, max val 65535
static inline void msg_dimmer_animation_set_timeoutsec(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 2, 16, val, bufx);
}

// Get TimeoutSec (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 2, length bits 16, min val 0, max val 65535
static inline uint32_t msg_dimmer_animation_get_timeoutsec(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 2, 16, 0, 65535, bufx);
}

// StartBrightness (UIntValue)
// Description: The brightness in percent at the beginning of the animation.

// Set StartBrightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 18, length bits 7, min val 0, max val 100
static inline void msg_dimmer_animation_set_startbrightness(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 18, 7, val, bufx);
}

// Get StartBrightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 18, length bits 7, min val 0, max val 100
static inline uint32_t msg_dimmer_animation_get_startbrightness(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 18, 7, 0, 100, bufx);
}

// EndBrightness (UIntValue)
// Description: The brightness in percent at the end of the animation.

// Set EndBrightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 25, length bits 7, min val 0, max val 100
static inline void msg_dimmer_animation_set_endbrightness(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 25, 7, val, bufx);
}

// Get EndBrightness (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 25, length bits 7, min val 0, max val 100
static inline uint32_t msg_dimmer_animation_get_endbrightness(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 25, 7, 0, 100, bufx);
}


// Message "dimmer_color"
// ----------------------
// MessageGroupID: 60
// MessageID: 10
// Possible MessageTypes: Get, Set, SetGet, Status, Ack, AckStatus
// Validity: test
// Length w/o Header + HeaderExtension: 6 bits
// Data fields: Color
// Description: This is to set a fixed color.

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_dimmer_color_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(60);
  pkg_headerext_get_set_messageid(10);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Set".
static inline void pkg_header_init_dimmer_color_set(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(1);
  pkg_headerext_set_set_messagegroupid(60);
  pkg_headerext_set_set_messageid(10);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 1;
}

// Function to initialize header for the MessageType "SetGet".
static inline void pkg_header_init_dimmer_color_setget(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(2);
  pkg_headerext_setget_set_messagegroupid(60);
  pkg_headerext_setget_set_messageid(10);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 2;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_dimmer_color_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(60);
  pkg_headerext_status_set_messageid(10);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "Ack".
static inline void pkg_header_init_dimmer_color_ack(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(9);
  __HEADEROFFSETBITS = 109;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 9;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_dimmer_color_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(60);
  pkg_headerext_ackstatus_set_messageid(10);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 10;
}

// Color (UIntValue)
// Description: The color is according to the 6 bit color palette used in SHC.

// Set Color (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 6, min val 0, max val 63
static inline void msg_dimmer_color_set_color(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 6, val, bufx);
}

// Get Color (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 6, min val 0, max val 63
static inline uint32_t msg_dimmer_color_get_color(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 6, 0, 63, bufx);
}


// Message "dimmer_coloranimation"
// -------------------------------
// MessageGroupID: 60
// MessageID: 11
// Possible MessageTypes: Get, Set, SetGet, Status, Ack, AckStatus
// Validity: test
// Length w/o Header + HeaderExtension: 115 bits
// Data fields: Repeat, AutoReverse, Time, Color
// Description: This is to set a color animation.

// Function to initialize header for the MessageType "Get".
static inline void pkg_header_init_dimmer_coloranimation_get(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(0);
  pkg_headerext_get_set_messagegroupid(60);
  pkg_headerext_get_set_messageid(11);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 0;
}

// Function to initialize header for the MessageType "Set".
static inline void pkg_header_init_dimmer_coloranimation_set(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(1);
  pkg_headerext_set_set_messagegroupid(60);
  pkg_headerext_set_set_messageid(11);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 1;
}

// Function to initialize header for the MessageType "SetGet".
static inline void pkg_header_init_dimmer_coloranimation_setget(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(2);
  pkg_headerext_setget_set_messagegroupid(60);
  pkg_headerext_setget_set_messageid(11);
  __HEADEROFFSETBITS = 95;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 2;
}

// Function to initialize header for the MessageType "Status".
static inline void pkg_header_init_dimmer_coloranimation_status(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(8);
  pkg_headerext_status_set_messagegroupid(60);
  pkg_headerext_status_set_messageid(11);
  __HEADEROFFSETBITS = 83;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 8;
}

// Function to initialize header for the MessageType "Ack".
static inline void pkg_header_init_dimmer_coloranimation_ack(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(9);
  __HEADEROFFSETBITS = 109;
  __PACKETSIZEBYTES = 16;
  __MESSAGETYPE = 9;
}

// Function to initialize header for the MessageType "AckStatus".
static inline void pkg_header_init_dimmer_coloranimation_ackstatus(void)
{
  memset(&bufx[0], 0, sizeof(bufx));
  pkg_header_set_messagetype(10);
  pkg_headerext_ackstatus_set_messagegroupid(60);
  pkg_headerext_ackstatus_set_messageid(11);
  __HEADEROFFSETBITS = 120;
  __PACKETSIZEBYTES = 32;
  __MESSAGETYPE = 10;
}

// Repeat (UIntValue)
// Description: The number of times the animation will be repeated. 0 means infinitely.

// Set Repeat (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 4, min val 0, max val 15
static inline void msg_dimmer_coloranimation_set_repeat(uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 0, 4, val, bufx);
}

// Get Repeat (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 0, length bits 4, min val 0, max val 15
static inline uint32_t msg_dimmer_coloranimation_get_repeat(void)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 0, 4, 0, 15, bufx);
}

// AutoReverse (BoolValue)
// Description: If true, the animation will be played back in the normal direction and then in reverse order.

// Set AutoReverse (BoolValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 4, length bits 1
static inline void msg_dimmer_coloranimation_set_autoreverse(bool val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 4, 1, val ? 1 : 0, bufx);
}

// Get AutoReverse (BoolValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 4, length bits 1
static inline bool msg_dimmer_coloranimation_get_autoreverse(void)
{
  return array_read_UIntValue8((uint16_t)__HEADEROFFSETBITS + 4, 1, 0, 1, bufx) == 1;
}

// Time (UIntValue[10])
// This sub-element with 5 bits is part of an element with 11 bits in a structured array.
// Description: The time for the animation between the current color and the next one. The number of seconds used is 0.05 * 1.3 ^ Time and covers the range from 0.03s to 170s. Use 0 to mark the end of the animation. Further values will be ignored.

// Set Time (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 5 + (uint16_t)index * 11, length bits 5, min val 0, max val 31
static inline void msg_dimmer_coloranimation_set_time(uint8_t index, uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 5 + (uint16_t)index * 11, 5, val, bufx);
}

// Get Time (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 5 + (uint16_t)index * 11, length bits 5, min val 0, max val 31
static inline uint32_t msg_dimmer_coloranimation_get_time(uint8_t index)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 5 + (uint16_t)index * 11, 5, 0, 31, bufx);
}

// Color (UIntValue[10])
// This sub-element with 6 bits is part of an element with 11 bits in a structured array.
// Description: The color is according to the 6 bit color palette used in SHC. The last color (or the first when AutoReverse is true) of the animation will remain visible after the animation is completed.

// Set Color (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 11, length bits 6, min val 0, max val 63
static inline void msg_dimmer_coloranimation_set_color(uint8_t index, uint32_t val)
{
  array_write_UIntValue((uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 11, 6, val, bufx);
}

// Get Color (UIntValue)
// Offset: (uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 11, length bits 6, min val 0, max val 63
static inline uint32_t msg_dimmer_coloranimation_get_color(uint8_t index)
{
  return array_read_UIntValue32((uint16_t)__HEADEROFFSETBITS + 10 + (uint16_t)index * 11, 6, 0, 63, bufx);
}

#endif /* _MSGGRP_DIMMER_H */
