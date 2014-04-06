#!/usr/bin/perl -w

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


package SHC_parser;

use strict;
use feature qw(switch);
use XML::LibXML;
use SHC_datafields;

# Hash for data field definitions.
my %dataFields = ();

# Hashes used to translate between names and IDs.
my %messageTypeID2messageTypeName = ();
my %messageTypeName2messageTypeID = ();

my %messageGroupID2messageGroupName = ();
my %messageGroupName2messageGroupID = ();

my %messageID2messageName = ();
my %messageName2messageID = ();

sub new
{
    my $class = shift;
    init_datafield_positions();
    my $self = {
        _senderID => 0,
		_packetCounter => 0,
		_messageType => 0,
		_messageGroupID => 0,
		_messageGroupName => "",
		_messageID => 0,
		_messageName => "",
		_messageData => "",
    };
    bless $self, $class;
    return $self;
}

sub init_datafield_positions()
{
	my $x = XML::LibXML->new() or die "new failed";
	my $d = $x->parse_file("packet_layout.xml") or die "parse failed";

	for my $element ($d->findnodes("/Packet/Header/EnumValue[ID='MessageType']/Element"))
	{
		my $value = ($element->findnodes("Value"))[0]->textContent;
		my $name = ($element->findnodes("Name"))[0]->textContent;
		
		$messageTypeID2messageTypeName{$value} = $name;
		$messageTypeName2messageTypeID{$name} = $value;
	}

	for my $messageGroup ($d->findnodes("/Packet/MessageGroup"))
	{
		my $messageGroupName = ($messageGroup->findnodes("Name"))[0]->textContent;
		my $messageGroupID = ($messageGroup->findnodes("MessageGroupID"))[0]->textContent;
		
		$messageGroupID2messageGroupName{$messageGroupID} = $messageGroupName;
		$messageGroupName2messageGroupID{$messageGroupName} = $messageGroupID;
		
		for my $message ($messageGroup->findnodes("Message"))
		{
			my $messageName = ($message->findnodes("Name"))[0]->textContent;
			my $messageID = ($message->findnodes("MessageID"))[0]->textContent;

			$messageID2messageName{$messageGroupID . "-" . $messageID} = $messageName;
			$messageName2messageID{$messageGroupName . "-" . $messageName} = $messageID;

			my $offset = 0;
			
			for my $field ($message->findnodes("UIntValue|IntValue|BoolValue|EnumValue"))
			{
				given($field->nodeName)
				{
					when('UIntValue')
					{
						my $id = ($field->findnodes("ID"))[0]->textContent;
						my $bits = ($field->findnodes("Bits"))[0]->textContent;
		
						print "Data field " . $id . " starts at " . $offset . " with " . $bits . " bits.\n";
		
						$dataFields{$messageGroupID . "-" . $messageID . "-" . $id} = new UIntValue($id, $offset, $bits);
		
						$offset += $bits;
					}
		
					when('IntValue')
					{
						my $id = ($field->findnodes("ID"))[0]->textContent;
						my $bits = ($field->findnodes("Bits"))[0]->textContent;
		
						print "Data field " . $id . " starts at " . $offset . " with " . $bits . " bits.\n";
		
						$dataFields{$messageGroupID . "-" . $messageID . "-" . $id} = new IntValue($id, $offset, $bits);
		
						$offset += $bits;
					}
		
					when('BoolValue')
					{
						my $id = ($field->findnodes("ID"))[0]->textContent;
						my $bits = 1;
		
						print "Data field " . $id . " starts at " . $offset . " with " . $bits . " bits.\n";
		
						$dataFields{$messageGroupID . "-" . $messageID . "-" . $id} = new BoolValue($id, $offset);
		
						$offset += $bits;
					}
					
					when('EnumValue')
					{
						my $id = ($field->findnodes("ID"))[0]->textContent;
						my $bits = ($field->findnodes("Bits"))[0]->textContent;
		
						print "Data field " . $id . " starts at " . $offset . " with " . $bits . " bits.\n";
		
						my $object = new EnumValue($id, $offset, $bits);
						$dataFields{$messageGroupID . "-" . $messageID . "-" . $id} = $object;
		
						for my $element ($field->findnodes("Element"))
						{
							my $value = ($element->findnodes("Value"))[0]->textContent;
							my $name = ($element->findnodes("Name"))[0]->textContent;
		
							$object->addValue($name, $value);
						}
		
						$offset += $bits;
					}
				}
			}
		}
	}
}

sub parse
{
	my ($self, $msg) = @_;

	if ($msg =~ /^Packet Data: SenderID=(\d*);PacketCounter=(\d*);MessageType=(\d*);MessageGroupID=(.*?);MessageID=(\d*);MessageData=([^;]*);.*/)
	{
		$self->{_senderID} = $1;
		$self->{_packetCounter} = $2;
		$self->{_messageType} = $3;
		$self->{_messageGroupID} = $4;
		$self->{_messageID} = $5;
		$self->{_messageData} = $6;		
	} else {
		#Log3 $hash, 4, "SHC_TEMP  ($msg) data error";
	}
}

sub getSenderID
{
	my ($self) = @_;
	return $self->{_senderID};
}

sub getMessageTypeName
{
	my ($self) = @_;
	return $messageTypeID2messageTypeName{$self->{_messageType}};
}

sub getMessageGroupName
{
	my ($self) = @_;
	return $messageGroupID2messageGroupName{$self->{_messageGroupID}};
}

sub getMessageName
{
	my ($self) = @_;
	return $messageID2messageName{$self->{_messageGroupID} . "-" . $self->{_messageID}};
}

sub getMessageData
{
	my ($self) = @_;
	return $self->{_messageData};
}

sub getField
{
	my ( $self, $fieldName ) = @_;
    
    my $obj = $dataFields{$self->{_messageGroupID} . "-" . $self->{_messageID} . "-" . $fieldName};
	my @dataArray = map hex("0x$_"), $self->{_messageData} =~ /(..)/g;

    return $obj->getValue(\@dataArray);
}

1;
