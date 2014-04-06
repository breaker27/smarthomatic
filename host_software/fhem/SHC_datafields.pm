#!/usr/bin/perl 

################################################################
# This file is part of the smarthomatic module for FHEM.
#
# Copyright (c) 2014 Uwe Freese
#
# You can find smarthomatic at www.smarthomatic.org.
# You can find FHEM at ww.fhem.de.
#
# This file is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with smarthomatic. If not, see <http://www.gnu.org/licenses/>.
################################################################

package SHC_util;

sub getUInt($$$)
{
	my ($byteArrayRef, $offset, $length_bits) = @_;
	
	my $byte = $offset / 8;
	my $bit = $offset % 8;
	
	my $byres_read = 0;
	my $val = 0;
	my $shiftBits;
	
	# read the bytes one after another, shift them to the correct position and add them
	while ($length_bits + $bit > $byres_read * 8)
	{
		$shiftBits = $length_bits + $bit - $byres_read * 8 - 8;
		my $zz = @$byteArrayRef[$byte + $byres_read];

		if ($shiftBits >= 0)
		{
			$val += $zz << $shiftBits;
		}
		else
		{
			$val += $zz >> - $shiftBits;
		}

		$byres_read++;
	}

	# filter out only the wanted bits and clear unwanted upper bits
	if ($length_bits < 32)
	{
		$val = $val & ((1 << $length_bits) - 1);
	}

	return $val;
}

sub getInt($$$)
{
	my ($byteArrayRef, $offset, $length_bits) = @_;
	
	# FIX ME! DOES NOT WORK WITH NEGATIVE VALUES!
	
	$x = getUInt($byteArrayRef, $offset, $length_bits);
	
	# If MSB is 1 (value is negative interpreted as signed int),
	# set all higher bits also to 1.
	if ((($x >> ($length_bits - 1)) & 1) == 1)
	{
		$x = $x | ~((1 << ($length_bits - 1)) - 1);
	}

	$y = $x;
	
	return $y;
}


package UIntValue;

sub new
{
    my $class = shift;
    my $self = {
        _id => shift,
        _offset => shift,
        _bits  => shift,
    };
    bless $self, $class;
    return $self;
}

sub getValue {
    my ($self, $byteArrayRef) = @_;

    return SHC_util::getUInt($byteArrayRef, $self->{_offset}, $self->{_bits});
}

package IntValue;

sub new
{
    my $class = shift;
    my $self = {
        _id => shift,
        _offset => shift,
        _bits  => shift,
    };
    bless $self, $class;
    return $self;
}

sub getValue {
    my ($self, $byteArrayRef) = @_;

    return SHC_util::getUInt($byteArrayRef, $self->{_offset}, $self->{_bits});
}

package BoolValue;

sub new
{
    my $class = shift;
    my $self = {
        _id => shift,
        _offset => shift,
    };
    bless $self, $class;
    return $self;
}

sub getValue {
    my ($self, $byteArrayRef) = @_;

    return SHC_util::getUInt($byteArrayRef, $self->{_offset}, 1) == 1 ? 1 : 0;
}

package EnumValue;

my %name2value = ();
my %value2name = ();

sub new
{
    my $class = shift;
    my $self = {
        _id => shift,
        _offset => shift,
        _bits  => shift,
    };
    bless $self, $class;
    return $self;
}

sub addValue {
    my ( $self, $name, $value ) = @_;
    
    $name2value{$name} = $value;
    $value2name{$value} = $name;
}


sub getValue {
    my ($self, $input) = @_;
    
    return 17;
}

1;
