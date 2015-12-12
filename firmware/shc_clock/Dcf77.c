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

#include <avr/io.h>
#include <util/atomic.h>
#include <util/parity.h>

#include "Dcf77.h"
#include "../src_common/util_hw.h"

enum DCF_INFO_BITS	{
	DCF_BIT_CALL =		0x01,		// 15 Rufbit für Alarmierung der PTB-Mitarbeiter
	DCF_BIT_TZ_CHANGE =	0x02,		// 16 Bitwert = 1 falls ein Wechsel von MEZ nach MESZ oder umgekehrt bevorsteht; Dauer der Anzeige: 1 Stunde
	DCF_BIT_MESZ =		0x04,		// 17 gültige Zeit = MEZ, falls Bit 17=0 und Bit 18=1
	DCF_BIT_MEZ =		0x08,		// 18 gültige Zeit = MESZ, falls Bit 17=1 und Bit 18=0
	DCF_BIT_LEAPSEC =	0x10,		// 19 Bitwert = 1 falls innerhalb den nächsten 59 Minuten eine Schaltsekunde angeordnet ist.
	//    Beim Einfügen einer Schaltsekunde wird anstelle der 59. die 60. Sekundenmarke weggelassen und in der 58. erfolgt ausnahmsweise ein Trägerabfall.
	DCF_BIT_START =		0x20,		// 20 Startbit für Zeitinformation (immer 1)
};

enum DCF_CTRL_FLAGS	{
	DCF_FLAG_PARITY =		0x01,	// Parity-Marker
	DCF_FLAG_PIN =			0x02,	// Pin-Marker for edge recognition
	DCF_FLAG_SUMMERTIME =	0x20,	// daylight saving last telegram, same bit position as DCF_ST_SUMMERTIME
};

enum DCF_STATE	{
	DCF_ST_RECEIVE =		0x01,	// receiving takes place
	DCF_ST_VALID =			0x02,	// valid time received
	DCF_ST_SET =			0x04,	// time has been set
	DCF_ST_INCREMENT =		0x08,	// manual increment of time
	
	DCF_ST_LASTOK =			0x10,	// last minute OK
	DCF_ST_SUMMERTIME =		0x20,	// daylight saving
	DCF_ST_BLINK50 =		0x80,	// for blinking ':'
};


static DateTime				CurrentDateTime;			// current time
static volatile uint8_t		Seconds = 0;				// current seconds, will be synchronized with DCF77 telegram
static volatile int8_t		LastError = DCF_OK;			// last error occured or DCF_OK if no.
static volatile uint8_t		State = 0;					// DCF77 core state, combination of DCF_STATE bits

inline uint8_t bcd2bin(uint8_t bcd)
{
	// The Hi-Nibble has to be divided by 16 and to be multiplied by 10.
	// This will be acheived by 4 bit right shift, multiplication b 2 and multiplication by 8.
	
	uint8_t hi = bcd & 0xf0;		// Hi-Nibble
	bcd &= 0x0f;					// Lo-Nibble
	hi >>= 1;						// equals hi= (hi >> 4 << 1) equals hi = hi/16*2
	bcd += hi;
	hi >>= 2;						// equals hi= (hi >> 4 << 3) equals hi = hi/16*8
	bcd += hi;
	return bcd;
}

void dcf77_timer100(bool pin)
{
	// DCF77 Core
	static uint8_t				sec100 = 1	;				// hundredths of a second 
	static uint8_t				pulseLen;					// measuring length of a pulse, unit 10ms
	static uint8_t				flags;						// combination of DCF_CTRL_FLAGS bits
	static uint8_t				bitMask;					// next bit to be stored or 0
	static uint8_t				dataByte;					// collects transferred bits 
	
	// Bit Debouncer
	static uint8_t				dbcBits = 0;				// needs to hold all bits (DBC_BIT_NUM)
	static uint8_t				dbcCount = 0;
	static int8_t				index = DCF_ERR_UNKNOWN;	// positive: second, negative: error (DCF_ERR_...)
	
	static DateTime				rcvDateTime;				// collects the currently received telegram (= next minute)
	static DateTime				lastDateTime;				// telegram of the previous minute if successfully received
	
	uint8_t st = State;

	// Standalone-Uhr
	if (--sec100 == 0)
	{
		st &= ~DCF_ST_BLINK50;
		sec100 = 100;
		uint8_t s = Seconds;
		if (++s == 60)
		{
			s = 0;
			st |= DCF_ST_INCREMENT;
		}
		Seconds = s;
	}
	else if (sec100 == 50)
	{
		st |= DCF_ST_BLINK50;
	}

	// DCF77
	// Der Beginn der Absenkung liegt jeweils auf dem Beginn der Sekunden 0 bis 58 innerhalb einer Minute.
	// In der 59. Sekunde erfolgt keine Absenkung, wodurch die nachfolgende Sekundenmarke den Beginn einer
	// Minute kennzeichnet und der Empfänger synchronisiert werden kann.

	if (pulseLen < 255) pulseLen++;

	// Bit-"GMV"
	if (dbcBits & (1U << DBC_BIT_NUM)) dbcCount--;
	dbcBits <<= 1;
	if (pin)
	{
		dbcCount++;
		dbcBits |= 1;
	}

	if ((flags & DCF_FLAG_PIN) == 0)
	{
		if (dbcCount >= (DBC_BIT_NUM - DBC_MAX_WRONG_BITS))		// Input High
		{
			// L/H edge
			flags |= DCF_FLAG_PIN;
			DCF_LED(1);
			if (index < 0) st &= ~(DCF_ST_RECEIVE | DCF_ST_LASTOK | DCF_ST_VALID);				// not synchronized
			else if ((pulseLen < 5) || (pulseLen > 25)) index = DCF_ERR_PULSE_LENGTH;			// error - pulse length
			else if (bitMask)
			{
				// store data bit
				if (pulseLen > 15) dataByte |= bitMask;			// log. 1 Puls
				else dataByte &= ~bitMask;						// log. 0 Puls
				bitMask <<= 1;									// prepare for next bit
			}
		}
	}
	else
	{
		if (dbcCount <= DBC_MAX_WRONG_BITS)						// Input Low
		{
			// H/L edge
			flags &= ~DCF_FLAG_PIN;
			DCF_LED(0);

			// new second
			if (pulseLen > 150)
			{
				// gap > 1.5 Sek.
				if (pulseLen > 250)
				{
					// error - no pulse
					index = DCF_ERR_NO_PULSE;
					st &= ~(DCF_ST_RECEIVE | DCF_ST_VALID);
				}
				else
				{
					st |= DCF_ST_RECEIVE;													// receiving takes place
					if (index == 58)
					{
						// a telegram has been received completely
						// check date parity
						if (dataByte & 1) flags ^= DCF_FLAG_PARITY;
						if ((flags & DCF_FLAG_PARITY))
						{
							index = DCF_ERR_PAR_DATE;
						}
						else
						{
							// date OK
							if (st & DCF_ST_LASTOK)
							{
								// at change of daylight saving correct the value of LastDateTime for compare
								if ((rcvDateTime.Minute == 0) && ((st ^ flags) & DCF_ST_SUMMERTIME))
								{
									if (st & DCF_ST_SUMMERTIME) lastDateTime.Hour += 1;		// winter -> summer
									else lastDateTime.Hour -= 1;							// summer -> winter
								}
								
								inc_date_time_minute(&lastDateTime);						// next minute for compare
								if (compare_date_time(&rcvDateTime, &lastDateTime))			// plausibility check
								{
									// take over time (minute, hour, date)
									CurrentDateTime = rcvDateTime;							
									st |= DCF_ST_VALID | DCF_ST_SET;
									st &= ~DCF_ST_INCREMENT;								// mo manual increment
									
									// synchronize seconds clock
									Seconds = 0;											
									sec100 = 100;
									
									// remember daylight saving of last telegram
									if (st & DCF_ST_SUMMERTIME) flags |= DCF_FLAG_SUMMERTIME;
									else flags &= ~DCF_FLAG_SUMMERTIME;
								}
							}
							lastDateTime = rcvDateTime;
							st |= DCF_ST_LASTOK;
						}
					}
					if (index > 0) LastError = 0;									// clear LastError
					if (index != 58) st &= ~DCF_ST_LASTOK;							// no valid time
					index = 0;
					invalidate_date_time(&rcvDateTime);
				}
			}
			else if ((index >= 0) && (index < 60))
			{
				index++;
			}
			pulseLen = 0;

			// evaluation received bit, preparing next bit
			switch (index)
			{
				case 0:
					// preparation minute start
					dataByte = 0;
					bitMask = 1;
					break;

				case 1:
					// evaluate minutes
					if (dataByte & 1) index = DCF_ERR_NO_BEGIN;								// error - minute start always 0

					// preparation - nothing
					bitMask = 0;
					break;

				case 15:
					// preparation info-flags
					dataByte = 0;
					bitMask = 1;
					break;

				case 21:
					// evaluate info flags
					if ((dataByte & DCF_BIT_START) == 0) index = DCF_ERR_NO_TIME_START;		// eror - no time start
					else
					{
						dataByte &= (DCF_BIT_MEZ | DCF_BIT_MESZ);
						switch (dataByte)
						{
							// 17 gültige Zeit = MEZ, falls Bit 17=0 und Bit 18=1
							// 18 gültige Zeit = MESZ, falls Bit 17=1 und Bit 18=0
							case DCF_BIT_MEZ:	st &= ~DCF_ST_SUMMERTIME; break;
							case DCF_BIT_MESZ:	st |= DCF_ST_SUMMERTIME; break;
							default: index = DCF_ERR_MEZ;									// error - undefined
						}
					}

					// preparation minute
					dataByte = 0;
					bitMask = 1;
					break;

				case 29:
					// evaluation minute
					if (parity_even_bit(dataByte)) index = DCF_ERR_PAR_MINUTE;				// error - parity
					else
					{
						dataByte = bcd2bin(dataByte & 0x7f);
						if (dataByte > 59) index = DCF_ERR_RANGE_MINUTE;					// error - range
						else rcvDateTime.Minute = dataByte;
					}

					// preparation hour
					dataByte = 0;
					bitMask = 1;
					break;

				case 36:
					// evaluation hour
					if (parity_even_bit(dataByte)) index = DCF_ERR_PAR_HOUR;				// error - parity
					else
					{
						dataByte = bcd2bin(dataByte & 0x3f);
						if (dataByte > 23) index = DCF_ERR_RANGE_HOUR;						// error - range
						else rcvDateTime.Hour = dataByte;
					}

					// preparation day
					dataByte = 0;
					bitMask = 1;
					break;

				case 42:
					// evaluation day
					flags &= ~DCF_FLAG_PARITY;
					if (parity_even_bit(dataByte)) flags ^= DCF_FLAG_PARITY;
					dataByte = bcd2bin(dataByte & 0x3f);
					if ((dataByte < 1) || (dataByte > 31)) index = DCF_ERR_RANGE_DAY;		// error - range
					else rcvDateTime.Day = dataByte;

					// preparation day of week
					dataByte = 0;
					bitMask = 1;
					break;

				case 45:
					// evaluation day of week
					if (parity_even_bit(dataByte)) flags ^= DCF_FLAG_PARITY;
					dataByte = bcd2bin(dataByte & 0x07);
					if ((dataByte < 1) || (dataByte > 7)) index = DCF_ERR_RANGE_WDAY;		// error - range
					else rcvDateTime.WDay = dataByte - 1;									// store as 0-6

					// preparation month
					dataByte = 0;
					bitMask = 1;
					break;

				case 50:
					// evaluation month
					if (parity_even_bit(dataByte)) flags ^= DCF_FLAG_PARITY;
					dataByte = bcd2bin(dataByte & 0x1f);
					if ((dataByte < 1) || (dataByte > 12)) index = DCF_ERR_RANGE_MONTH;		// error - range
					else rcvDateTime.Month = dataByte;

					// preparation year
					dataByte = 0;
					bitMask = 1;
					break;

				case 58:
					// evaluation year
					if (parity_even_bit(dataByte)) flags ^= DCF_FLAG_PARITY;
					dataByte = bcd2bin(dataByte);
					if (dataByte < 12) index = DCF_ERR_RANGE_YEAR;							// error - range
					else rcvDateTime.Year = dataByte;

					// preparation parity date
					dataByte = 0;
					bitMask = 1;
					break;

				case 59:					// only at leap second
					index--;				// ignore
					// preparation - nothing
					bitMask = 0;
					break;
			}
		}
	}
	
	if (st & DCF_ST_INCREMENT)
	{
		if (is_date_time_valid(&CurrentDateTime))
		{
			inc_date_time_minute(&CurrentDateTime);
			st |= DCF_ST_SET;
		}
		st &= ~DCF_ST_INCREMENT;
	}
	
	State = st;
	if (index < 0) LastError = index;
} 

bool dcf77_get_current(DateTime* dt)
{
	if (State & DCF_ST_SET)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			*dt = *((volatile DateTime*)&CurrentDateTime);
			State &= ~DCF_ST_SET;
		}
		return true;
	}
	return false;
}

bool dcf77_get_blink_flag(void)
{ 
	return (State & DCF_ST_BLINK50) != 0; 
}
	
uint8_t dcf77_get_seconds(void)
{
	return Seconds;	
}

int8_t dcf77_get_last_error(void)		
{ 
	return LastError; 
}

DCF_RCV_STATE dcf77_get_rcv_state(void)
{
	uint8_t st = State;
	if (st & DCF_ST_LASTOK) return DCF_RCV_OK;
	if (st & DCF_ST_RECEIVE) return DCF_RCV_PROGRESS;
	return DCF_RCV_NO_SIGNAL;
}

bool dcf77_is_summer_time(void)
{
	uint8_t st = State;
	return (st & DCF_ST_SUMMERTIME) != 0;
}
