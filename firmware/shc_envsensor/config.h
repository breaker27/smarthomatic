/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
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
*/

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

// SHT11 hum/temp sensor

#define SHT11_AT_5V         4010
#define SHT11_AT_4V         3980
#define SHT11_AT_3_5V       3970
#define SHT11_AT_3V         3960
#define SHT11_AT_2_5V       3940

#define SHT11_TEMP_V_COMP   SHT11_AT_3V

#define SHT11_RES_LOW       1  //8_12_Bit
#define SHT11_RES_HIGH      0  //12_14_Bit
#define SHT11_RESOLUTION    SHT11_RES_HIGH

#define SHT11_PORT	        C
#define SHT11_SCL	        (1<<PC2) // was: PC4!
#define SHT11_SDA	        (1<<PC3)
#define SHT11_LOWRES	    SHT11_RESOLUTION	


#define GLUE(a, b)	a##b
#define PORT(x)		GLUE(PORT, x)
#define PIN(x)		GLUE(PIN, x)
#define DDR(x)		GLUE(DDR, x)

#define setBits(port,mask)	do{ (port) |=  (mask); }while(0)
#define clrBits(port,mask)	do{ (port) &= ~(mask); }while(0)
#define tstBits(port,mask)	((port) & (mask))
