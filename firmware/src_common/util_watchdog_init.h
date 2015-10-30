/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2015 Uwe Freese
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

// Include this file only ONCE in the main c file.

#include <avr/wdt.h>

uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

// Function Pototype
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

// Function Implementation
void wdt_init(void)
{
	mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();
	return;
}
