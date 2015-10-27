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

#include "DateTime.h"

static inline uint8_t is_leap(uint8_t year)
{
	return (year % 4) == 0;
}

static uint8_t days_in_month(uint8_t month, uint8_t year)
{
	switch (month)
	{
		case 4: case 6: case 9: case 11: 
			return 30;
		case 2: 
			return is_leap(year) ? 29 : 28;
			
		default: 
			return 31;
	}
}

void inc_date_time_minute(DateTime* dt)
{
	if (++dt->Minute == 60)
	{
		dt->Minute = 0;
		
		if (++dt->Hour == 24)
		{
			dt->Hour = 0;
			
			if (++dt->WDay == 7)
			{
				dt->WDay = 0;
			}
			
			if (++dt->Day > days_in_month(dt->Month, dt->Year))
			{
				dt->Day = 1;
				
				if (++dt->Month > 12)
				{
					dt->Month = 1;
					
					if (++dt->Year == 100)
					{
						dt->Year = 0;
					}
				}
			}
		}
	}
}

#if 0
void dec_date_time_hour(DateTime* dt)
{
	int h = dt->Hour;
	if (--h < 0)
	{
		h+= 24;
		int wd = dt->WDay;
		if (--wd < 0)
		{
			wd+= 7;
		}
		dt->WDay = (uint8_t)wd;
		
		if (dt->Day && dt->Month)
		{
			if (--dt->Day == 0)
			{
				if (--dt->Month == 0)
				{
					dt->Month = 12;
					
					int y = dt->Year;
					if (--y < 0)
					{
						y+= 100;
					}
					dt->Year = (uint8_t)y;
				}
				dt->Day = days_in_month(dt->Month, dt->Year);
			}
		}
	}
	dt->Hour = (uint8_t)h;
}
#endif

void invalidate_date_time(DateTime* dt)
{
	dt->Day = 0;
	dt->Month = 0;
}
