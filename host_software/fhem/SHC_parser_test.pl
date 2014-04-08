#!/usr/bin/perl -w

##########################################################################
# This is a test program for the smarthomatic module for FHEM.
#
# Copyright (c) 2014 Uwe Freese
#
# You can find smarthomatic at www.smarthomatic.org.
# You can find FHEM at www.fhem.de.
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
##########################################################################


use strict;
use feature qw(switch);
use SHC_parser;

my $parser = new SHC_parser();


# parse a receiced packet string

sub parser_test($)
{
	my $str = shift();
	
	$parser->parse($str);
	
	print "\nParsing Message: " . $str . "\n\n";
	
	print "SenderID: " . $parser->getSenderID() . "\n";
	print "MessageTypeName: " . $parser->getMessageTypeName() . "\n";
	print "MessageGroupName: " . $parser->getMessageGroupName() . "\n";
	print "MessageName: " . $parser->getMessageName() . "\n";
	print "MessageData: " . $parser->getMessageData() . "\n";

	# process message and react on content
	
	my $grp = $parser->getMessageGroupName();
	my $msg = $parser->getMessageName();
	
	given($grp)
	{
		when('Generic')
		{
			given($msg)
			{
				when('BatteryStatus')
				{
					print "Percentage: " . $parser->getField("Percentage") . "\n";
				}
				when('Version')
				{
					print "Major: " . $parser->getField("Major") . "\n";
					print "Minor: " . $parser->getField("Minor") . "\n";
					print "Patch: " . $parser->getField("Patch") . "\n";
					print "Hash: " . $parser->getField("Hash") . "\n";
				}
			}
		}
		when('EnvSensor')
		{
			given($msg)
			{
				when('TempHumBriStatus')
				{
					print "Temperature: " . $parser->getField("Temperature") . "\n";
					print "Humidity: " . $parser->getField("Humidity") . "\n";
					print "Brightness: " . $parser->getField("Brightness") . "\n";
				}
			}
		}
		when('PowerSwitch')
		{
			given($msg)
			{
				when('SwitchState')
				{
					print "On: " . $parser->getField("On") . "\n";
					print "TimeoutSec: " . $parser->getField("TimeoutSec") . "\n";
				}
			}
		}
	}
}

parser_test("Packet Data: SenderID=22;PacketCounter=101;MessageType=8;MessageGroupID=0;MessageID=5;MessageData=b40000000000;Percentage=3321;");
parser_test("Packet Data: SenderID=20;PacketCounter=19284;MessageType=8;MessageGroupID=10;MessageID=1;MessageData=085c79630000;Temperature=21.40;Humidity=48.5;Brightness=70;");
parser_test("Packet Data: SenderID=40;PacketCounter=7401;MessageType=8;MessageGroupID=20;MessageID=1;MessageData=000000000000;On=0;TimeoutSec=0;");
parser_test("Packet Data: SenderID=40;PacketCounter=7402;MessageType=8;MessageGroupID=0;MessageID=1;MessageData=00000000000000000000000000000000000000000000;Major=0;Minor=0;Patch=0;Hash=00000000;");


## Create message string for sending

$parser->initPacket("PowerSwitch", "SwitchState", "Set");
$parser->setField("PowerSwitch", "SwitchState", "TimeoutSec", 8);

print "BaseStation command = " . $parser->getSendString(61) . "\n";

$parser->setField("PowerSwitch", "SwitchState", "On", 1);
print "BaseStation command = " . $parser->getSendString(61) . "\n";
