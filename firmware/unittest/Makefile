#
# This file is part of smarthomatic, http://www.smarthomatic.org.
# Copyright (c) 2013 Uwe Freese
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

# Object files directory
OBJDIR = build
BINDIR = bin

# Compiler command
CC = gcc

# Linker command
LD = gcc

# Flags to pass to the compiler - add "-g" to include debug information
CFLAGS = -Wall
CFLAGS += -DUNITTEST=1 # tell some header files that we are compiling for the unittest (and AVR functions are not available)

# Flags to pass to the linker
LDFLAGS = 

# Command used to delete files
RM = rm

# List your source files here
CSRC = main.c ../src_common/e2p_access.c

# List your object files here
OBJ = $(CSRC:%.c=$(OBJDIR)/%.o)

# Define your compile target here.
PROG = $(BINDIR)/shc_unittest.exe

# Compile everything.
all: $(PROG)

# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the program
$(PROG): $(OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJ) -o $(PROG)

clean:
	$(RM) $(PROG)
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/depend

$(OBJDIR)/depend:
	$(CC) $(CFLAGS) -MM $(CSRC) > $(OBJDIR)/depend

run: all
	$(PROG)

include $(OBJDIR)/depend
