##########################################################################
# This file is part of the smarthomatic module for FHEM.
#
# Copyright (c) 2014 Stefan Baumann
#               2014, 2015, 2019, 2022 Uwe Freese
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
###########################################################################
# $Id: 37_SHCdev.pm 26457 2022-09-30 19:32:11Z breaker27 $

package main;

use strict;
use feature qw(switch);
use warnings;
use SetExtensions;
use Encode qw(encode_utf8 decode_utf8);

use SHC_parser;

my $parser = new SHC_parser();

my %dev_state_icons = (
  "PowerSwitch"         => ".*1\\d{7}:on:off .*0\\d{7}:off:on set.*:light_question:off",
  "Dimmer"              => "on:on off:off set.*:light_question:off",
  "EnvSensor"           => undef,
  "Controller"          => undef,
  "RGBDimmer"           => undef,
  "SoilMoistureMeter"   => ".*H:\\s\\d\\..*:ampel_rot"
);

my %web_cmds = (
  "PowerSwitch"         => "on:off:toggle:statusRequest",
  "Dimmer"              => "on:off:statusRequest",
  "EnvSensor"           => undef,
  "Controller"          => undef,
  "RGBDimmer"           => undef,
  "SoilMoistureMeter"   => undef
);

# Array format: [ reading1, str_format1, reading2, str_format2 ... ]
# "on" reading translates 0 -> "off"
#                         1 -> "on"
my %dev_state_format = (
  "PowerSwitch"         => ["port", "Port: "],
  "Dimmer"              => ["on", "", "brightness", "B: "],
  "EnvSensor"           => [    # Results in "T: 23.4 H: 27.3 Baro: 978.34 B: 45"
    "temperature",         "T: ",
    "humidity",            "H: ",
    "barometric_pressure", "Baro: ",
    "brightness",          "B: ",
    "distance",            "D: ",
    "port",                "Port: ",
    "ains",                "Ain: "
  ],
  "Controller"          => [
    "color",               "Color: ",
    "brightness",          "Brightness: "
  ],
  "RGBDimmer"           => [
    "color",               "Color: ",
    "brightness",          "Brightness: "
  ],
  "SoilMoistureMeter"   => ["humidity", "H: "]
);

# Supported set commands
# use "" if no set commands are available for device type
# use "cmd_name:cmd_additional_info"
#     cmd_additional_info: Description available at http://www.fhemwiki.de/wiki/DevelopmentModuleIntro#X_Set
my %sets = (
  "PowerSwitch"         => "on:noArg off:noArg toggle:noArg statusRequest:noArg " .
                           # Used from SetExtensions.pm
                           "blink on-for-timer on-till off-for-timer off-till intervals " .
                           "DigitalPort " .
                           "DigitalPortTimeout " .
                           "DigitalPin " .
                           "DigitalPinTimeout",
  "Dimmer"              => "on:noArg off:noArg toggle:noArg statusRequest:noArg pct:slider,0,1,100 ani " .
                           # Used from SetExtensions.pm
                           "blink on-for-timer on-till off-for-timer off-till intervals",
  "EnvSensor"           => "",
  "Controller"          => "Color " .
                           "ColorAnimation " .
                           "Dimmer.Brightness:slider,0,1,100 " .
                           "Text " .
                           "Tone " .
                           "Melody",
  "RGBDimmer"           => "Color " .
                           "ColorAnimation " .
                           "Dimmer.Brightness:slider,0,1,100 " .
                           "Tone " .
                           "Melody",
  "SoilMoistureMeter"   => "",
  "Custom"              => "Dimmer.Brightness " .
                           "Dimmer.Animation"
);

# Supported get commands
# use syntax from set commands
my %gets = (
  "PowerSwitch" => "",
  "Dimmer"      => "",
  "EnvSensor"   => "din:all,1,2,3,4,5,6,7,8 ain:all,1,2,3,4,5 ain_volt:1,2,3,4,5",
  "Controller"  => "",
  "RGBDimmer"   => "",
  "Custom"      => ""
);

sub SHCdev_Parse($$);

#####################################
sub SHCdev_Initialize($)
{
  my ($hash) = @_;

  $hash->{Match}    = "^PKT:SID=([1-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6]);";
  $hash->{SetFn}    = "SHCdev_Set";
  $hash->{GetFn}    = "SHCdev_Get";
  $hash->{DefFn}    = "SHCdev_Define";
  $hash->{UndefFn}  = "SHCdev_Undef";
  $hash->{ParseFn}  = "SHCdev_Parse";
  $hash->{AttrList} = "IODev"
                       ." readonly:1"
                       ." forceOn:1"
                       ." $readingFnAttributes"
                       ." devtype:EnvSensor,Dimmer,PowerSwitch,Controller,RGBDimmer,SoilMoistureMeter";
}

#####################################
sub SHCdev_Define($$)
{
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if (@a < 3 || @a > 4) {
    my $msg = "wrong syntax: define <name> SHCdev <SenderID> [<AesKey>] ";
    Log3 undef, 2, $msg;
    return $msg;
  }

  # Correct SenderID for SHC devices is from 1 - 4096 (leading zeros allowed)
  $a[2] =~ m/^([1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6])$/i;
  return "$a[2] is not a valid SHCdev SenderID" if (!defined($1));

  my $aeskey;

  if (@a == 3) {
    $aeskey = 0;
  } else {
    return "$a[3] is not a valid SHCdev AesKey" if ($a[3] lt 0 || $a[3] gt 15);
    $aeskey = $a[3];
  }

  my $name = $a[0];
  my $addr = $a[2];

  return "SHCdev device $addr already used for $modules{SHCdev}{defptr}{$addr}->{NAME}." if ($modules{SHCdev}{defptr}{$addr}
    && $modules{SHCdev}{defptr}{$addr}->{NAME} ne $name);

  $hash->{addr}   = $addr;
  $hash->{aeskey} = $aeskey;

  $modules{SHCdev}{defptr}{$addr} = $hash;

  AssignIoPort($hash);
  if (defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device";
  }

  return undef;
}

#####################################
sub SHCdev_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete($modules{SHCdev}{defptr}{$addr});

  return undef;
}

#####################################
sub SHCdev_Parse($$)
{
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};

  if (!$parser->parse($msg)) {
    Log3 $name, 1, "$name: Parser error: $msg";
    return "";
  }

  my $msgtypename   = $parser->getMessageTypeName();
  my $msggroupname  = $parser->getMessageGroupName();
  my $msgname       = $parser->getMessageName();
  my $packetcounter = $parser->getPacketCounter();
  my $raddr         = $parser->getSenderID();
  my $rhash         = $modules{SHCdev}{defptr}{$raddr};
  my $rname         = $rhash ? $rhash->{NAME} : $raddr;

  if (!$modules{SHCdev}{defptr}{$raddr}) {
    Log3 $name, 3, "$name: Unknown device $rname, please define it";
    return "UNDEFINED SHCdev_$rname SHCdev $raddr";
  }

  if (($msgtypename ne "Status") && ($msgtypename ne "AckStatus")) {
    Log3 $name, 3, "$rname: Ignoring MessageType $msgtypename";
    return "";
  }

  Log3 $name, 4, "$rname: Msg: $msg";
  Log3 $name, 4, "$rname: MsgType: $msgtypename, MsgGroupName: $msggroupname, MsgName: $msgname";

  my @list;
  push(@list, $rname);
  $rhash->{SHCdev_lastRcv} = TimeNow();
  $rhash->{SHCdev_msgtype} = "$msggroupname : $msgname : $msgtypename";

  my $readonly = AttrVal($rname, "readonly", "0");

  readingsBeginUpdate($rhash);

  # remember PacketCounter (which every message provides)
  readingsBulkUpdate($rhash, "packetCounter", $packetcounter);

  if ($msggroupname eq "Generic") {
    if ($msgname eq "Version") {
      my $major = $parser->getField("Major");
      my $minor = $parser->getField("Minor");
      my $patch = $parser->getField("Patch");
      my $vhash = $parser->getField("Hash");

      readingsBulkUpdate($rhash, "version", "$major.$minor.$patch-$vhash");
    } elsif ($msgname eq "DeviceInfo") {
      my $devtype = $parser->getField("DeviceType");
      my $major = $parser->getField("VersionMajor");
      my $minor = $parser->getField("VersionMinor");
      my $patch = $parser->getField("VersionPatch");
      my $vhash = $parser->getField("VersionHash");

      # Assign device type
      my $devtypeOld = AttrVal( $rname, "devtype", undef );
      if (!defined($devtypeOld)) {
        $attr{$rname}{devtype} = $devtype;
        Log3 $name, 3, "$rname: Assign device type = " . $attr{$rname}{devtype};
      }

      readingsBulkUpdate($rhash, "version", "$major.$minor.$patch-$vhash");
    } elsif ($msgname eq "HardwareError") {
      readingsBulkUpdate($rhash, "hardwareErrorCode", $parser->getField("ErrorCode"));
    } elsif ($msgname eq "BatteryStatus") {
      readingsBulkUpdate($rhash, "battery", $parser->getField("Percentage"));
    }
  } elsif ($msggroupname eq "GPIO") {
    if ($msgname eq "DigitalPortTimeout") {
      my $pins = "";
      for (my $i = 0 ; $i < 8 ; $i++) {
        my $pinx = $parser->getField("On", $i);
        my $timeoutx = $parser->getField("TimeoutSec", $i);
        my $channel = $i + 1;
        if ($channel == 1) {
          readingsBulkUpdate($rhash, "on", $pinx);
        }
        readingsBulkUpdate($rhash, "pin" . $channel, $pinx);
        readingsBulkUpdate($rhash, "timeout" . $channel, $timeoutx);
        $pins .= $pinx;
      }
      readingsBulkUpdate($rhash, "port", $pins);
    } elsif ($msgname eq "DigitalPort") {
      my $pins = "";
      for (my $i = 0 ; $i < 8 ; $i++) {
        my $pinx = $parser->getField("On", $i);
        my $channel = $i + 1;
        if ($channel == 1) {
          readingsBulkUpdate($rhash, "on", $pinx);
        }
        readingsBulkUpdate($rhash, "pin" . $channel, $pinx);
        $pins .= $pinx;
      }
      readingsBulkUpdate($rhash, "port", $pins);
    } elsif ($msgname eq "AnalogPort") {
      my $pins = "";
      for (my $i = 0 ; $i < 5 ; $i++) {
        my $pinx_on = $parser->getField("On", $i);
        my $pinx_volt = $parser->getField("Voltage", $i);
        my $channel = $i + 1;
        readingsBulkUpdate($rhash, "ain" . $channel, $pinx_on);
        readingsBulkUpdate($rhash, "ain_volt" . $channel, $pinx_volt);
        $pins .= $pinx_on;
      }
      readingsBulkUpdate($rhash, "ains", $pins);
    }
  } elsif ($msggroupname eq "Weather") {
    if ($msgname eq "Temperature") {
      my $tmp = $parser->getField("Temperature") / 100;    # parser returns centigrade

      readingsBulkUpdate($rhash, "temperature", $tmp);
    } elsif ($msgname eq "HumidityTemperature") {
      my $hum = $parser->getField("Humidity") / 10;        # parser returns 1/10 percent
      my $tmp = $parser->getField("Temperature") / 100;    # parser returns centigrade

      readingsBulkUpdate($rhash, "humidity",    $hum);
      readingsBulkUpdate($rhash, "temperature", $tmp);
    } elsif ($msgname eq "BarometricPressureTemperature") {
      my $bar = $parser->getField("BarometricPressure") / 100;    # parser returns pascal, use hPa
      my $tmp = $parser->getField("Temperature") / 100;           # parser returns centigrade

      readingsBulkUpdate($rhash, "barometric_pressure", $bar);
      readingsBulkUpdate($rhash, "temperature",         $tmp);
    } elsif ($msgname eq "Humidity") {
      my $hum = $parser->getField("Humidity") / 10;        # parser returns 1/10 percent

      readingsBulkUpdate($rhash, "humidity",    $hum);
    }
  } elsif ($msggroupname eq "Environment") {
    if ($msgname eq "Brightness") {
      my $brt = $parser->getField("Brightness");
      readingsBulkUpdate($rhash, "brightness", $brt);
    } elsif ($msgname eq "Distance") {
      my $brt = $parser->getField("Distance");
      readingsBulkUpdate($rhash, "distance", $brt);
    } elsif ($msgname eq "ParticulateMatter") {
      my $size = $parser->getField("TypicalParticleSize");

      if ($size != 1023) { # 1023 means invalid
        readingsBulkUpdate($rhash, "typicalParticleSize", $size / 100); # value was in 1/100 µm
      }

      for (my $i = 0 ; $i < 5 ; $i++) {
        $size = $parser->getField("Size", $i);

        if ($size) { # 0 means array element not used
          my $pmStr = int($size / 10) . "." . ($size % 10);
          my $massConcentration = $parser->getField("MassConcentration", $i);
          my $numberConcentration = $parser->getField("NumberConcentration", $i);

          if ($massConcentration != 1023) { # 1023 means invalid
            readingsBulkUpdate($rhash, "massConcentration_PM" . $pmStr, $massConcentration / 10); # value was in 1/10 µm
          }

          if ($numberConcentration != 4095) { # 4095 means invalid
            readingsBulkUpdate($rhash, "numberConcentration_PM" . $pmStr, $numberConcentration / 10); # value was in 1/10 µm
          }
        }
      }
    }
  } elsif ($msggroupname eq "Controller") {
    if ($msgname eq "MenuSelection") {
      my $index;

      for (my $i = 0 ; $i < 16 ; $i = $i + 1) {
        $index = $parser->getField("Index" , $i);
        # index 0 indicates the value was not changed / doesn't exist
        if ($index != 0) {
          readingsBulkUpdate($rhash, sprintf("index%02d", $i), $index);
		}
      }
    }
  } elsif ($msggroupname eq "Audio") {
    if ($msgname eq "Tone") {
      my $tone = $parser->getField("Tone");

      readingsBulkUpdate($rhash, "tone",         $tone);
    } elsif ($msgname eq "Melody") {
      my $repeat = $parser->getField("Repeat");
      my $autoreverse = $parser->getField("AutoReverse");
      readingsBulkUpdate($rhash, "repeat", $repeat);
      readingsBulkUpdate($rhash, "autoreverse", $autoreverse);
      for (my $i = 0 ; $i < 25 ; $i = $i + 1) {
        my $time  = $parser->getField("Time" , $i);
        my $effect = $parser->getField("Effect", $i);
        my $tone = $parser->getField("Tone", $i);
        readingsBulkUpdate($rhash, sprintf("time%02d", $i), $time);
        readingsBulkUpdate($rhash, sprintf("effect%02d", $i), $effect);
        readingsBulkUpdate($rhash, sprintf("tone%02d", $i), $tone);
      }
    }
  } elsif ($msggroupname eq "Dimmer") {
    if ($msgname eq "Brightness") {
      my $brightness = $parser->getField("Brightness");
      my $on = $brightness == 0 ? 0 : 1;

      readingsBulkUpdate($rhash, "on",         $on);
      readingsBulkUpdate($rhash, "brightness", $brightness);
    } elsif ($msgname eq "Color") {
      my $color = $parser->getField("Color");
      readingsBulkUpdate($rhash, "color", $color);
    } elsif ($msgname eq "ColorAnimation") {
      my $repeat = $parser->getField("Repeat");
      my $autoreverse = $parser->getField("AutoReverse");
      readingsBulkUpdate($rhash, "repeat", $repeat);
      readingsBulkUpdate($rhash, "autoreverse", $autoreverse);
      for (my $i = 0 ; $i < 10 ; $i = $i + 1) {
        my $time  = $parser->getField("Time" , $i);
        my $color = $parser->getField("Color", $i);
        readingsBulkUpdate($rhash, "time$i", $time);
        readingsBulkUpdate($rhash, "color$i", $color);
      }
    }
  }

  # If the devtype is defined add, if not already done, the according webCmds and devStateIcons
  my $devtype2 = AttrVal( $rname, "devtype", undef );
  if (defined($devtype2)) {
    if (!defined($attr{$rname}{devStateIcon}) && defined($dev_state_icons{$devtype2})) {
      $attr{$rname}{devStateIcon} = $dev_state_icons{$devtype2};
    }
    if (!defined($attr{$rname}{webCmd}) && defined($web_cmds{$devtype2})) {
      $attr{$rname}{webCmd} = $web_cmds{$devtype2};
    }
  }

  # Assemble state string according to %dev_state_format
  my $devtype3 = AttrVal( $rname, "devtype", undef );
  if (defined($devtype3) && defined($dev_state_format{$devtype3})) {
    my $state_format_arr = $dev_state_format{$devtype3};

    # Iterate over state_format array, if readings are available append it to the state string
    my $state_str = "";
    for (my $i = 0 ; $i < @$state_format_arr ; $i = $i + 2) {
      if ( defined($rhash->{READINGS}{$state_format_arr->[$i]})
        && defined($rhash->{READINGS}{$state_format_arr->[$i]}{VAL}))
      {
        my $val = $rhash->{READINGS}{$state_format_arr->[$i]}{VAL};

        if ($state_str ne "") {
          $state_str .= " ";
        }

        # "on" reading requires a special treatment because 0 translates to off, 1 translates to on
        if ($state_format_arr->[$i] eq "on") {
          $state_str .= $val == 0 ? "off" : "on";
        } else {
          $state_str .= $state_format_arr->[$i + 1] . $val;
        }
      }
    }
    readingsBulkUpdate($rhash, "state", $state_str);
  }

  readingsEndUpdate($rhash, 1);    # Do triggers to update log file

  return @list;
}

#####################################
sub SHCdev_Set($@)
{
  my ($hash, $name, @aa) = @_;
  my $cnt = @aa;

  my $cmd  = $aa[0];
  my $arg  = $aa[1];
  my $arg2 = $aa[2];
  my $arg3 = $aa[3];
  my $arg4 = $aa[4];

  return "\"set $name\" needs at least one parameter" if ($cnt < 1);

  # Return list of device-specific set-commands.
  # This list is used to provide the set commands in the web interface
  my $devtype = AttrVal( $name, "devtype", undef );
  if ($cmd eq "?") {
    if (!defined($devtype)) {
      return;
    } else {
      return $sets{$devtype};
    }
  }

  if (!defined($devtype)) {
    return "devtype not yet specifed. Currently supported device types are " . join(", ", sort keys %sets);
  }

  if (!defined($sets{$devtype})) {
    return "No set commands for " . $devtype . "device type supported ";
  }

  # TODO:
  # Currently the commands for every device type are defined in %sets but not used to verify the commands. Instead
  # the SetExtension.pm modulesis used for this purpose.
  # For more sophisticated device types this has to be revisited

  my $readonly = AttrVal($name, "readonly", "0");

  if ($devtype eq "PowerSwitch") {

    # Timeout functionality for SHCdev is not implemented, because FHEMs internal notification system
    # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

    if ($cmd eq 'toggle') {
      $cmd = ReadingsVal($name, "on", "0") eq "0" ? "on" : "off";
    }

    if (!$readonly && $cmd eq 'off') {
      readingsSingleUpdate($hash, "state", "set-$cmd", 1);
      $parser->initPacket("GPIO", "DigitalPin", "SetGet");
      $parser->setField("GPIO", "DigitalPin", "Pos", 0);
      $parser->setField("GPIO", "DigitalPin", "On", 0);
      SHCdev_Send($hash);
    } elsif (!$readonly && $cmd eq 'on') {
      readingsSingleUpdate($hash, "state", "set-$cmd", 1);
      $parser->initPacket("GPIO", "DigitalPin", "SetGet");
      $parser->setField("GPIO", "DigitalPin", "Pos", 0);
      $parser->setField("GPIO", "DigitalPin", "On", 1);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'statusRequest') {
      $parser->initPacket("GPIO", "DigitalPin", "Get");
      SHCdev_Send($hash);
    } elsif ($cmd eq 'DigitalPort') {
      $parser->initPacket("GPIO", "DigitalPort", "SetGet");
      # if not enough (less than 8) pinbits are available use zero as default
      my $pinbits = $arg . "00000000";
      for (my $i = 0 ; $i < 8 ; $i = $i + 1) {
        $parser->setField("GPIO", "DigitalPort", "On", substr($pinbits, $i , 1), $i);
      }
      SHCdev_Send($hash);
    } elsif ($cmd eq 'DigitalPortTimeout') { # TODO implement correctly
      $parser->initPacket("GPIO", "DigitalPortTimeout", "SetGet");
      # if not enough (less than 8) pinbits are available use zero as default
      my $pinbits = $arg . "00000000";
      for (my $i = 0 ; $i < 8 ; $i = $i + 1) {
        my $pintimeout = "0";   # default value for timeout
        if (exists  $aa[$i + 2]) {
          $pintimeout = $aa[$i + 2];
        }
        Log3 $name, 3, "$name: $i: Pin: " . substr($pinbits, $i , 1) . " Timeout: $pintimeout";
        $parser->setField("GPIO", "DigitalPortTimeout", "On", substr($pinbits, $i , 1), $i);
        $parser->setField("GPIO", "DigitalPortTimeout", "TimeoutSec", $pintimeout, $i);
      }
      SHCdev_Send($hash);
    } elsif ($cmd eq 'DigitalPin') {
      $parser->initPacket("GPIO", "DigitalPin", "SetGet");
      $parser->setField("GPIO", "DigitalPin", "Pos", $arg);
      $parser->setField("GPIO", "DigitalPin", "On", $arg2);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'DigitalPinTimeout') {
      $parser->initPacket("GPIO", "DigitalPinTimeout", "SetGet");
      $parser->setField("GPIO", "DigitalPinTimeout", "Pos", $arg);
      $parser->setField("GPIO", "DigitalPinTimeout", "On", $arg2);
      $parser->setField("GPIO", "DigitalPinTimeout", "TimeoutSec", $arg3);
      SHCdev_Send($hash);
    } else {
      return SetExtensions($hash, "", $name, @aa);
    }
  } elsif ($devtype eq "Dimmer") {

    # Timeout functionality for SHCdev is not implemented, because FHEMs internal notification system
    # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

    if ($cmd eq 'toggle') {
      $cmd = ReadingsVal($name, "state", "on") eq "off" ? "on" : "off";
    }

    if (!$readonly && $cmd eq 'off') {
      readingsSingleUpdate($hash, "state", "set-$cmd", 1);
      $parser->initPacket("Dimmer", "Brightness", "SetGet");
      $parser->setField("Dimmer", "Brightness", "Brightness", 0);
      SHCdev_Send($hash);
    } elsif (!$readonly && $cmd eq 'on') {
      readingsSingleUpdate($hash, "state", "set-$cmd", 1);
      $parser->initPacket("Dimmer", "Brightness", "SetGet");
      $parser->setField("Dimmer", "Brightness", "Brightness", 100);
      SHCdev_Send($hash);
    } elsif (!$readonly && $cmd eq 'pct') {
      my $brightness = $arg;

      # DEBUG
      # Log3 $name, 3, "$name: Args: $arg, $arg2, $arg3, $brightness";

      readingsSingleUpdate($hash, "state", "set-pct:$brightness", 1);
      $parser->initPacket("Dimmer", "Brightness", "SetGet");
      $parser->setField("Dimmer", "Brightness", "Brightness", $brightness);
      SHCdev_Send($hash);
    } elsif (!$readonly && $cmd eq 'ani') {

      #TODO Verify argument values
      my $brightness = $arg;

      # DEBUG
      # Log3 $name, 3, "$name: ani args: $arg, $arg2, $arg3, $arg4, $brightness";

      readingsSingleUpdate($hash, "state", "set-ani", 1);
      $parser->initPacket("Dimmer", "Animation", "SetGet");
      $parser->setField("Dimmer", "Animation", "AnimationMode",   $arg);
      $parser->setField("Dimmer", "Animation", "TimeoutSec",      $arg2);
      $parser->setField("Dimmer", "Animation", "StartBrightness", $arg3);
      $parser->setField("Dimmer", "Animation", "EndBrightness",   $arg4);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'statusRequest') {
      $parser->initPacket("Dimmer", "Brightness", "Get");
      SHCdev_Send($hash);
    } else {
      return SetExtensions($hash, "", $name, @aa);
    }
  } elsif (($devtype eq "Controller") || ($devtype eq "RGBDimmer")) {
    if ($cmd eq 'Color') {
      #TODO Verify argument values
      my $color = $arg;

      # DEBUG
      # Log3 $name, 3, "$name: Color args: $arg, $arg2, $arg3, $arg4";

      readingsSingleUpdate($hash, "state", "set-color:$color", 1);
      $parser->initPacket("Dimmer", "Color", "SetGet");
      $parser->setField("Dimmer", "Color", "Color",   $color);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'Tone') {
      #TODO Verify argument values
      my $tone = $arg;

      # DEBUG
      # Log3 $name, 3, "$name: Tone args: $arg, $arg2, $arg3, $arg4";

      readingsSingleUpdate($hash, "state", "set-tone:$tone", 1);
      $parser->initPacket("Audio", "Tone", "SetGet");
      $parser->setField("Audio", "Tone", "Tone",   $tone);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'ColorAnimation') {
      #TODO Verify argument values

      $parser->initPacket("Dimmer", "ColorAnimation", "SetGet");
      $parser->setField("Dimmer", "ColorAnimation", "Repeat", $arg);
      $parser->setField("Dimmer", "ColorAnimation", "AutoReverse", $arg2);

      my $curtime = 0;
      my $curcolor = 0;
      # Iterate over all given command line parameters and set Time and Color
      # accordingly. Fill the remaining values with zero.
      for (my $i = 0 ; $i < 10 ; $i = $i + 1) {
        if (!defined($aa[($i * 2) + 3])) {
          $curtime = 0;
        } else {
          $curtime = $aa[($i * 2) + 3];
        }
        if (!defined($aa[($i * 2) + 4])) {
          $curcolor = 0;
        } else {
          $curcolor = $aa[($i * 2) + 4];
        }

        # DEBUG
        # Log3 $name, 3, "$name: Nr: $i Time: $curtime Color: $curcolor";

        $parser->setField("Dimmer", "ColorAnimation", "Time" , $curtime, $i);
        $parser->setField("Dimmer", "ColorAnimation", "Color", $curcolor, $i);
      }
      readingsSingleUpdate($hash, "state", "set-coloranimation", 1);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'Melody') {
      #TODO Verify argument values

      $parser->initPacket("Audio", "Melody", "SetGet");
      $parser->setField("Audio", "Melody", "Repeat", $arg);
      $parser->setField("Audio", "Melody", "AutoReverse", $arg2);

      my $curtime = 0;
      my $cureffect = 0;
      my $curtone = 0;
      # Iterate over all given command line parameters and set Time, Effect and Tone
      # accordingly. Fill the remaining values with zero.
      for (my $i = 0 ; $i < 25 ; $i = $i + 1) {
        if (!defined($aa[($i * 3) + 3])) {
          $curtime = 0;
        } else {
          $curtime = $aa[($i * 3) + 3];
        }
        if (!defined($aa[($i * 3) + 4])) {
          $cureffect = 0;
        } else {
          $cureffect = $aa[($i * 3) + 4];
        }
        if (!defined($aa[($i * 3) + 5])) {
          $curtone = 0;
        } else {
          $curtone = $aa[($i * 3) + 5];
        }

        # DEBUG
        # Log3 $name, 3, "$name: Nr: $i Time: $curtime Effect: $cureffect Tone: $curtone";

        $parser->setField("Audio", "Melody", "Time" , $curtime, $i);
        $parser->setField("Audio", "Melody", "Effect", $cureffect, $i);
        $parser->setField("Audio", "Melody", "Tone", $curtone, $i);
      }
      readingsSingleUpdate($hash, "state", "set-melody", 1);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'Dimmer.Brightness') {
      my $brightness = $arg;

      # DEBUG
      # Log3 $name, 3, "$name: Args: $arg, $arg2, $arg3, $brightness";

      readingsSingleUpdate($hash, "state", "set-brightness:$brightness", 1);
      $parser->initPacket("Dimmer", "Brightness", "SetGet");
      $parser->setField("Dimmer", "Brightness", "Brightness", $brightness);
      SHCdev_Send($hash);
    } elsif ($cmd eq 'Text') {
      $parser->initPacket("Display", "Text", "SetGet");
      $parser->setField("Display", "Text", "PosY", $arg);
      $parser->setField("Display", "Text", "PosX", $arg2);
      $parser->setField("Display", "Text", "Format", $arg3);
      $arg4 = decode_utf8($arg4);
      $arg4 =~ s/(?<!\\)_/ /g; # replace non-escaped '_' with space
      $arg4 =~ s/\\_/_/g;      # replace escape character from escaped '_'
      $arg4 =~ s/(?<!\\)\\1/\x01/g; # replace \1 with character 1 (user character)
      $arg4 =~ s/(?<!\\)\\2/\x02/g; # replace \2 with character 2 (user character)
      $arg4 =~ s/(?<!\\)\\3/\x03/g; # replace \3 with character 3 (user character)
      $arg4 =~ s/(?<!\\)\\4/\x04/g; # replace \4 with character 4 (user character)
      $arg4 =~ s/(?<!\\)\\5/\x05/g; # replace \5 with character 5 (user character)
      $arg4 =~ s/(?<!\\)\\6/\x06/g; # replace \6 with character 6 (user character)
      $arg4 =~ s/(?<!\\)\\7/\x07/g; # replace \7 with character 7 (user character)
      $arg4 =~ s/(?<!\\)\\8/\x08/g; # replace \8 with character 8 (user character)
      $parser->setField("Display", "Text", "Text", $arg4);
      SHCdev_Send($hash);
    } else {
      return SetExtensions($hash, "", $name, @aa);
    }
  }

  return undef;
}

#####################################
sub SHCdev_Get($@)
{
  my ($hash, $name, @aa) = @_;
  my $cnt = @aa;

  my $cmd = $aa[0];
  my $arg = $aa[1];

  return "\"get $name\" needs at least one parameter" if ($cnt < 1);

  my $devtype = AttrVal( $name, "devtype", undef );
  if (!defined($devtype)) {
    return "\"devtype\" not yet specifed. Currently supported device types are " . join(", ", sort keys %sets);
  }

  if (!defined($gets{$devtype})) {
    return "No get commands for " . $devtype . " device type supported ";
  }

  if ($devtype eq "EnvSensor") {
    if ($cmd eq 'din') {
      if ($arg =~ /[1-8]/) {
        my $channel = "din" . $arg;
        if ( defined($hash->{READINGS}{$channel})
          && defined($hash->{READINGS}{$channel}{VAL}))
        {
          return "$name.$channel => " . $hash->{READINGS}{$channel}{VAL};
        }
        return "Error: \"input " . $channel . "\" readings not yet available or not supported by device";
      }
      elsif ($arg eq "all")
      {
        if ( defined($hash->{READINGS}{port})
          && defined($hash->{READINGS}{port}{VAL}))
        {
          return "$name.port => " . $hash->{READINGS}{port}{VAL};
        }
        return "Error: \"input all\" readings not yet available or not supported by device";
      }
    }
    if ($cmd eq 'ain') {
      if ($arg =~ /[1-5]/) {
        my $channel = "ain" . $arg;
        if ( defined($hash->{READINGS}{$channel})
          && defined($hash->{READINGS}{$channel}{VAL}))
        {
          return "$name.$channel => " . $hash->{READINGS}{$channel}{VAL};
        }
        return "Error: \"input " . $channel . "\" readings not yet available or not supported by device";
      }
      elsif ($arg eq "all")
      {
        if ( defined($hash->{READINGS}{ains})
          && defined($hash->{READINGS}{ains}{VAL}))
        {
          return "$name.ains => " . $hash->{READINGS}{ains}{VAL};
        }
        return "Error: \"input all\" readings not yet available or not supported by device";
      }
    }
    if ($cmd eq 'ain_volt') {
      if ($arg =~ /[1-5]/) {
        my $channel = "ain_volt" . $arg;
        if ( defined($hash->{READINGS}{$channel})
          && defined($hash->{READINGS}{$channel}{VAL}))
        {
          return "$name.$channel => " . $hash->{READINGS}{$channel}{VAL};
        }
        return "Error: \"input " . $channel . "\" readings not yet available or not supported by device";
      }
    }

    # This return is required to provide the get commands in the web interface
    return "Unknown argument $cmd, choose one of " . $gets{$devtype};
  }
  return undef;
}

#####################################
sub SHCdev_Send($)
{
  my ($hash) = @_;
  my $name = $hash->{NAME};

  $hash->{SHCdev_lastSend} = TimeNow();

  my $msg = $parser->getSendString($hash->{addr}, $hash->{aeskey});

  Log3 $name, 3, "$name: Sending $msg";

  IOWrite($hash, $msg);
}

1;

=pod
=item summary    support of several smarthomatic devices (www.smarthomatic.org)
=item summary_DE Unterstützung verschiedener smarthomatic-Geräte (www.smarthomatic.org)
=begin html

<a name="SHCdev"></a>
<h3>SHCdev</h3>
<ul>
  SHC is the device module that supports several device types available
  at <a href="http://www.smarthomatic.org">www.smarthomatic.org</a>.<br><br>

  These device are connected to the FHEM server through the SHC base station (<a href="#SHC">SHC</a>).<br><br>
  Currently supported are:<br>
  <ul>
    <li>EnvSensor</li>
    <li>PowerSwitch</li>
    <li>Dimmer</li>
    <li>Controller</li>
    <li>RGBDimmer</li>
    <li>SoilMoistureMeter</li>
  </ul><br>

  <a name="SHCdev_Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; SHCdev &lt;SenderID&gt; [&lt;AesKey&gt;]</code><br>
    <br>
    &lt;SenderID&gt;<br>
    is a number ranging from 0 .. 4095 to identify the SHCdev device.<br><br>

    &lt;AesKey&gt;<br>
    is a optional number ranging from 0 .. 15 to select an encryption key.
    It is required for the basestation to communicate with remote devides
    The default value is 0.<br><br>

    Note: devices are autocreated on reception of the first message.<br>
  </ul>
  <br>

  <a name="SHCdev_Set"></a>
  <b>Set</b>
  <ul>
    <li>on<br>
        Supported by Dimmer and PowerSwitch (on always refers to pin1).
    </li><br>
    <li>off<br>
        Supported by Dimmer and PowerSwitch (off always refers to pin1).
    </li><br>
    <li>pct &lt;0..100&gt;<br>
        Sets the brightness in percent. Supported by Dimmer.
    </li><br>
    <li>ani &lt;AnimationMode&gt; &lt;TimeoutSec&gt; &lt;StartBrightness&gt; &lt;EndBrightness&gt;<br>
        Description and details available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Dimmer_Animation">www.smarthomatic.org</a>.
        Supported by Dimmer.
    </li><br>
    <li>statusRequest<br>
        Supported by Dimmer and PowerSwitch.
    </li><br>
    <li>Color &lt;ColorNumber&gt;<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Dimmer_Color">www.smarthomatic.org</a>.
        The color palette can be found <a href="http://www.smarthomatic.org/devices/rgb_dimmer.html">here</a>
        Supported by RGBDimmer.
    </li><br>
    <li>ColorAnimation &lt;Repeat&gt; &lt;AutoReverse&gt; &lt;Time0&gt; &lt;ColorNumber0&gt; &lt;Time1&gt; &lt;ColorNumber1&gt; ... up to 10 time/color pairs<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Dimmer_ColorAnimation">www.smarthomatic.org</a>.
        The color palette can be found <a href="http://www.smarthomatic.org/devices/rgb_dimmer.html">here</a>
        Supported by RGBDimmer.
    </li><br>
    <li>Tone &lt;ToneNumber&gt;<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Audio_Tone">www.smarthomatic.org</a>.
        The tone definition can be found <a href="http://www.smarthomatic.org/devices/rgb_dimmer.html">here</a>
        Supported by RGBDimmer.
    </li><br>
    <li>Melody &lt;Repeat&gt; &lt;AutoReverse&gt; &lt;Time0&gt; &lt;Effect0&gt; &lt;ToneNumber0&gt; &lt;Time1&gt; &lt;Effect1&gt; &lt;ToneNumber1&gt; ... up to 25 time/effect/tone pairs<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Audio_Melody">www.smarthomatic.org</a>.
        The tone definition can be found <a href="http://www.smarthomatic.org/devices/rgb_dimmer.html">here</a>
        Supported by RGBDimmer.
    </li><br>
    <li>Text &lt;PosY&gt; &lt;PosX&gt; &lt;Format&gt; &lt;Text&gt;<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#Display_Text">www.smarthomatic.org</a>. Supported by Controller.<br/>
        <b>Note:</b> Since FHEM parameters can't include spaces, there is a special form to enter them.
        To add a space to the text, use the underline character (e.g. 'Hello_world').
        If you want to send an underline character, escape it with the backslash (e.g. '\_test\_').
    </li><br>
    <li>DigitalPin &lt;Pos&gt; &lt;On&gt;<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#GPIO_DigitalPin">www.smarthomatic.org</a>.
        Supported by PowerSwitch.
    </li><br>
    <li>DigitalPinTimeout &lt;Pos&gt; &lt;On&gt; &lt;Timeout&gt;<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#GPIO_DigitalPinTimeout">www.smarthomatic.org</a>.
        Supported by PowerSwitch.
    </li><br>
    <li>DigitalPort &lt;On&gt;<br>
        &lt;On&gt;<br>
        is a bit array (0 or 1) describing the port state. If less than eight bits were provided zero is assumed.
        Example: set SHC_device DigitalPort 10110000 will set pin0, pin2 and pin3 to 1.<br>
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#GPIO_DigitalPort">www.smarthomatic.org</a>.
        Supported by PowerSwitch.
    </li><br>
    <li>DigitalPortTimeout &lt;On&gt; &lt;Timeout0&gt; .. &lt;Timeout7&gt;<br>
        &lt;On&gt;<br>
        is a bit array (0 or 1) describing the port state. If less than eight bits were provided zero is assumed.
        Example: set SHC_device DigitalPort 10110000 will set pin0, pin2 and pin3 to 1.<br>
        &lt;Timeout0&gt; .. &lt;Timeout7&gt;<br>
        are the timeouts for each pin. If no timeout is provided zero is assumed.
        A detailed description is available at <a href="http://www.smarthomatic.org/basics/message_catalog.html#GPIO_DigitalPortTimeout">www.smarthomatic.org</a>.
        Supported by PowerSwitch.
    </li><br>
    <li><a href="#setExtensions"> set extensions</a><br>
        Supported by Dimmer and PowerSwitch.</li>
  </ul><br>

  <a name="SHCdev_Get"></a>
  <b>Get</b>
  <ul>
    <li>din &lt;pin&gt;<br>
        Returns the state of the specified digital input pin for pin = 1..8. Or the state of all pins for pin = all.
        Supported by EnvSensor.
    </li><br>
    <li>ain &lt;pin&gt;<br>
        Returns the state of the specified analog input pin for pin = 1..5. Or the state of all pins for pin = all.
        If the voltage of the pin is over the specied trigger threshold) it return 1 otherwise 0.
        Supported by EnvSensor.
    </li><br>
    <li>ain &lt;pin&gt;<br>
        Returns the state of the specified analog input pin for pin = 1..5. Or the state of all pins for pin = all.
        If the voltage of the pin is over the specied trigger threshold) it return 1 otherwise 0.
        Supported by EnvSensor.
    </li><br>
    <li>ain_volt &lt;pin&gt;<br>
        Returns the voltage of the specified analog input pin for pin = 1..5 in millivolts, ranging from 0 .. 1100 mV.
        Supported by EnvSensor.
    </li><br>
  </ul><br>

  <a name="SHCdev_Attr"></a>
  <b>Attributes</b>
  <ul>
    <li>devtype<br>
      The device type determines the command set, default web commands and the
      default devStateicon. Currently supported are: EnvSensor, Dimmer, PowerSwitch, RGBDimmer, SoilMoistureMeter.<br><br>

      Note: If the device is not set manually, it will be determined automatically
      on reception of a device type specific message. For example: If a
      temperature message is received, the device type will be set to EnvSensor.
    </li><br>
    <li>readonly<br>
        if set to a value != 0 all switching commands (on, off, toggle, ...) will be disabled.
    </li><br>
    <li>forceOn<br>
        try to switch on the device whenever an off status is received.
    </li><br>
  </ul><br>
</ul>

=end html
=cut
