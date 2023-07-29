/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2022 Uwe Freese
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

// The fuses mentioned here are set to "0" (= "programmed").
// Fuses not mentioned are left to "1" (= "unprogrammed").

FUSES =
{
	// value 0xD7 - settings for full swing crystal oscillator (20 MHz), BOD enabled
	.low = (FUSE_SUT1 & FUSE_CKSEL3),
	// value 0xD1 - EESAVE is 0, others are default
	.high = (FUSE_SPIEN & FUSE_EESAVE & FUSE_BOOTSZ1 & FUSE_BOOTSZ0),
	// value 0xFC - set BOD to 4.3V to prevent accidentially FLASH garbage - BOD1 + BOD0
	//.extended = (FUSE_BODLEVEL1 & FUSE_BODLEVEL0),
	// value 0xFD - set BOD to 2.7V to prevent accidentially FLASH garbage - BOD1
	.extended = (FUSE_BODLEVEL1),
};
