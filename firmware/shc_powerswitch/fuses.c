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

#include <avr/io.h> 

FUSES = 
{ 
	// value 0xE2 - use internal oscillator with 8 MHz
    .low = (FUSE_CKSEL0 & FUSE_CKSEL2 & FUSE_CKSEL3 & FUSE_SUT0),
	// value 0xD6 - set EESAVE, set BOD to 1.8V to prevent accidentially
	// FLASH garbage, others are default
	.high = (FUSE_EESAVE & FUSE_SPIEN & FUSE_BODLEVEL0),
	// should be value F9 at ATMega168
    .extended = EFUSE_DEFAULT, 
};
