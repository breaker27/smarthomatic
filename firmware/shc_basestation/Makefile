#
# This file is part of smarthomatic, http://www.smarthomatic.org.
# Copyright (c) 2013..2015 Uwe Freese
#
# smarthomatic is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# smarthomatic is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with smarthomatic. If not, see <http://www.gnu.org/licenses/>.
#

# CPU config
MCU		       = atmega328
F_CPU	       = 20000000
# baud rate in makefile is ignored, because it is configured by e2p
UART_BAUD_RATE = 19200

# never remove this, because the base station needs the UART
UART_DEBUG     = 1
UART_RX        = 1

# Target file name (without extension).
TARGET = shc_basestation

# Source files (C dependencies are automatically generated).
#   C			*.c
#   Assembler	*.S
CSRC = $(TARGET).c rfm12.c ../src_common/util.c ../src_common/uart.c ../src_common/aes256.c fuses.c request_buffer.c
ASRC = ../src_common/aes_keyschedule-asm.S ../src_common/aes_enc-asm.S ../src_common/aes_dec-asm.S ../src_common/aes_sbox-asm.S ../src_common/aes_invsbox-asm.S
FUSES = -U hfuse:w:hfuse.hex:i -U lfuse:w:lfuse.hex:i -U efuse:w:efuse.hex:i -u

#******************************************************************************
# Include defaults
#******************************************************************************
# Compiler / Linker options
include ../makefile_common/Makefile.options
# General configuration
include ../makefile_common/Makefile.config
# Include make targets
include ../makefile_common/Makefile.targets
# Include make rules
include ../makefile_common/Makefile.rules
# Include the dependency files.
-include $(wildcard $(OBJDIR)/*.d)

