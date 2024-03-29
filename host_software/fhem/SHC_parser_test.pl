#!/usr/bin/perl -w

##########################################################################
# This is a test program for the smarthomatic module for FHEM.
#
# Copyright (c) 2014..2019 Uwe Freese
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
				when('HardwareError')
				{
					print "ErrorCode: " . $parser->getField("ErrorCode") . "\n";
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
		when('GPIO')
		{
			given($msg)
			{
				when('DigitalPin')
				{
					print "Pin " . $parser->getField("Pos") . ": " . $parser->getField("On") . "\n";
				}
				when('DigitalPort')
				{
					for (my $i = 0; $i < 8; $i++)
					{
						print "Pin " . $i . ": " . $parser->getField("On", $i) . "\n";
					}
				}
				when('AnalogPin')
				{
					for (my $i = 0; $i < 8; $i++)
					{
						print "Pin " . $i . ": " . $parser->getField("Voltage", $i) . "mV = " . $parser->getField("On", $i) . "\n";
					}
				}
			}
		}
		when('Weather')
		{
			given($msg)
			{
				when('HumidityTemperature')
				{
					print "Humidity: " . $parser->getField("Humidity") . "\n";
					print "Temperature: " . ($parser->getField("Temperature") / 100) . "\n";
				}
				when('Temperature')
				{
					print "Temperature: " . ($parser->getField("Temperature") / 100) . "\n";
				}
			}
		}
		when('Environment')
		{
			given($msg)
			{
				when('ParticulateMatter')
				{
					print "TypicalParticleSize RAW: " . $parser->getField("TypicalParticleSize") . "\n";

					for (my $i = 0; $i < 5; $i++)
					{
						print "Size RAW " . $i . ": " . $parser->getField("Size", $i) . "\n";
						print "MassConcentration RAW " . $i . ": " . $parser->getField("MassConcentration", $i) . "\n";
						print "NumberConcentration RAW " . $i . ": " . $parser->getField("NumberConcentration", $i) . "\n";
					}
				}
			}
		}
	}
}

parser_test("PKT:SID=27;PC=301;MT=8;MGID=1;MID=1;MD=800000000000;");
parser_test("PKT:SID=22;PC=101;MT=8;MGID=0;MID=5;MD=b40000000000;");
parser_test("PKT:SID=20;PC=19284;MT=8;MGID=10;MID=1;MD=085c79630000;");
parser_test("PKT:SID=40;PC=7401;MT=8;MGID=20;MID=1;MD=000000000000;");
parser_test("PKT:SID=40;PC=7402;MT=8;MGID=0;MID=1;MD=00000000000000000000000000000000000000000000;");
parser_test("PKT:SID=23;PC=414;MT=8;MGID=1;MID=2;MD=0042e000000000000000000000000000000000000000;");
parser_test("PKT:SID=23;PC=307;MT=8;MGID=1;MID=2;MD=80458000000000000000000000000000000000000000;");
parser_test("PKT:SID=71;PC=527;MT=8;MGID=10;MID=2;MD=a67161000000;");
parser_test("PKT:SID=27;PC=35106;MT=8;MGID=1;MID=1;MD=800000000000;");
parser_test("PKT:SID=21;PC=680;MT=8;MGID=10;MID=1;MD=de7b00000000;");
parser_test("PKT:SID=42;PC=103;MT=8;MGID=0;MID=3;MD=00;");
parser_test("PKT:SID=37;PC=5115;MT=8;MGID=11;MID=3;MD=0f017ff1880a0e873064411d22811074d90441d3;");
parser_test("PKT:SID=45;PC=100;MT=1;MGID=40;MID=1;MD=0140414243;");

# Create message string for sending

$parser->initPacket("GPIO", "DigitalPort", "Set");
$parser->setField("GPIO", "DigitalPort", "On", 1, 0);
$parser->setField("GPIO", "DigitalPort", "On", 1, 2);
$parser->setField("GPIO", "DigitalPort", "On", 1, 4);
$parser->setField("GPIO", "DigitalPort", "On", 1, 6);

print "BaseStation command = " . $parser->getSendString(61) . "\n";

$parser->initPacket("GPIO", "DigitalPinTimeout", "SetGet");
$parser->setField("GPIO", "DigitalPinTimeout", "Pos", 3);
$parser->setField("GPIO", "DigitalPinTimeout", "On", 1);
$parser->setField("GPIO", "DigitalPinTimeout", "TimeoutSec", 8);

print "BaseStation command = " . $parser->getSendString(61) . "\n";

$parser->initPacket("Dimmer", "ColorAnimation", "Set");
$parser->setField("Dimmer", "ColorAnimation", "Repeat", 3);
$parser->setField("Dimmer", "ColorAnimation", "Time", 10, 0);
$parser->setField("Dimmer", "ColorAnimation", "Color", 0, 0);
$parser->setField("Dimmer", "ColorAnimation", "Time", 16, 1);
$parser->setField("Dimmer", "ColorAnimation", "Color", 48, 1);
$parser->setField("Dimmer", "ColorAnimation", "Time", 16, 2);
$parser->setField("Dimmer", "ColorAnimation", "Color", 12, 2);
$parser->setField("Dimmer", "ColorAnimation", "Time", 16, 3);
$parser->setField("Dimmer", "ColorAnimation", "Color", 3, 3);
$parser->setField("Dimmer", "ColorAnimation", "Time", 10, 4);
$parser->setField("Dimmer", "ColorAnimation", "Color", 48, 4);

print "BaseStation command = " . $parser->getSendString(50) . "\n";

$parser->initPacket("Display", "Text", "Set");
$parser->setField("Display", "Text", "PosY", 0);
$parser->setField("Display", "Text", "PosX", 10);
$parser->setField("Display", "Text", "Text", "ABC");

print "BaseStation command = " . $parser->getSendString(45) . "\n";
