These files are part of smarthomatic, http://www.smarthomatic.org.
Copyright (c) 2013 Uwe Freese

smarthomatic is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

smarthomatic is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with smarthomatic. If not, see <http://www.gnu.org/licenses/>.


Flash your ATMega
=================

You can flash your ATMega with the fuses, firmware and EEPROM file as
follows:

avrdude -p m328p -U lfuse:w:lfuse.bin:r -U hfuse:w:hfuse.bin:r
   -U efuse:w:efuse.bin:r
avrdude -p m328p -U flash:w:shc_basestation.hex
avrdude -p m328p -U eeprom:w:shc_basestation.e2p

Use device type "m328p" for the ATMega 328 and "m168" for the ATMega 168.
Please read the wiki at http://www.smarthomatic.org/wiki about more
information.
