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

#ifndef DATETIME_H_
#define DATETIME_H_

#include <stdbool.h>

typedef struct _DateTime {
	uint8_t		Minute;			// 0 .. 59
	uint8_t		Hour;			// 0 .. 23
	uint8_t		WDay;			// 0 = Mo, 6 = Su
	uint8_t		Day;			// 1 .. 31
	uint8_t		Month;			// 1 .. 12
	uint8_t		Year;			// 00 .. 99
} DateTime;


extern void inc_date_time_minute(DateTime* dt);
//extern void dec_date_time_hour(DateTime* dt);
extern void invalidate_date_time(DateTime* dt);

static inline bool is_date_time_valid(const DateTime* dt)
{
	return dt->Day && dt->Month;
}

static inline bool compare_date_time(const DateTime* dt1, const DateTime* dt2)
{
	return
		dt1->Minute == dt2->Minute &&
		dt1->Hour == dt2->Hour &&
		dt1->WDay == dt2->WDay &&
		dt1->Day == dt2->Day &&
		dt1->Month == dt2->Month &&
		dt1->Year == dt2->Year;
}

#endif /* DATETIME_H_ */
