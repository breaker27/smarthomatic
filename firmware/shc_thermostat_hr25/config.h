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

#define GLUE(a, b)	a##b
#define PORT(x)		GLUE(PORT, x)
#define PIN(x)		GLUE(PIN, x)
#define DDR(x)		GLUE(DDR, x)

#define setBits(port,mask)	do{ (port) |=  (mask); }while(0)
#define clrBits(port,mask)	do{ (port) &= ~(mask); }while(0)
#define tstBits(port,mask)	((port) & (mask))
