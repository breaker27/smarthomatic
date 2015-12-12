/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Andreas Richter (original author of DCF77 library)
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
*/

#ifndef DCF77_H_
#define DCF77_H_

#include <stdbool.h>

#include "DateTime.h"

/* Bit-"GMV" aus DBC_BIT_NUM Bits, Wertebereich von DebounceBits beachten! */
#define DBC_BIT_NUM 7

/* höchstens soviele Bits dürfen falsch sein damit vom Debouncer eine Schaltschwelle erkannt wird */
#define DBC_MAX_WRONG_BITS 2

#define DCF_LED(on) switch_led(!on)

typedef enum _DCF_ERRORS	{
	DCF_ERR_RANGE_MINUTE=	-16,
	DCF_ERR_RANGE_HOUR=		-15,
	DCF_ERR_RANGE_WDAY=		-14,
	DCF_ERR_RANGE_DAY=		-13,
	DCF_ERR_RANGE_MONTH=	-12,
	DCF_ERR_RANGE_YEAR=		-11,
	
	DCF_ERR_PAR_MINUTE=		-10,
	DCF_ERR_PAR_HOUR=		-9,
	DCF_ERR_PAR_WDAY=		-8,
	DCF_ERR_PAR_DATE=		-7,
	
	DCF_ERR_PULSE_LENGTH=	-6,
	DCF_ERR_NO_PULSE=		-5,
	DCF_ERR_MEZ=			-4,
	DCF_ERR_NO_BEGIN=		-3,
	DCF_ERR_NO_TIME_START=	-2,
	DCF_ERR_UNKNOWN=		-1,
	DCF_OK = 0,
} DCF_ERRORS;

typedef enum _DCF_RCV_STATE { 
	DCF_RCV_NO_SIGNAL, 
	DCF_RCV_PROGRESS, 
	DCF_RCV_OK, 
} DCF_RCV_STATE;

extern void dcf77_timer100(bool pin);
extern bool dcf77_is_summer_time(void);
extern bool dcf77_get_current(DateTime* dt);
extern uint8_t dcf77_get_seconds(void);
extern bool dcf77_get_blink_flag(void);
extern int8_t dcf77_get_last_error(void);

extern DCF_RCV_STATE dcf77_get_rcv_state(void);
#endif /* DCF77_H_ */
