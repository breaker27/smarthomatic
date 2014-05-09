# Copied from 36_PCA301.pm and adapted
# $Id: 36_SHC_Dev.pm 3934 2013-09-21 09:21:39Z justme1968 $
#
# TODO:

package main;

use strict;
use feature qw(switch);
use warnings;
use SetExtensions;

use SHC_parser;

my $parser = new SHC_parser();

my %dev_state_icons = (
  "PowerSwitch" => "on:on:toggle off:off:toggle set.*:light_question:off",
  "Dimmer"      => "on:on off:off set.*:light_question:off",
  "EnvSensor"   => undef
);

my %web_cmds = (
  "PowerSwitch" => "on:off:toggle:statusRequest",
  "Dimmer"      => "on:off:statusRequest",
  "EnvSensor"   => undef
);

# Array format: [ reading1, str_format1, reading2, str_format2 ... ]
# "on" reading translates 0 -> "off"
#                         1 -> "on"
my %dev_state_format = (
  "PowerSwitch" => ["on", ""],
  "Dimmer"      => ["on", "", "brightness", "B: "],
  "EnvSensor" => [    # Results in "T: 23.4 H: 27.3 Baro: 978.34 B: 45"
    "temperature",         "T: ",
    "humidity",            "H: ",
    "barometric_pressure", "Baro: ",
    "brightness",          "B: "
  ]
);

# Supported Set commands
my %sets = (
  "PowerSwitch" => ["on", "off", "toggle", "statusRequest",
                    # Used from SetExtensions.pm
                    "blink", "on-for-timer", "on-till", "off-for-timer", "off-till", "intervals"],
  "Dimmer"      => ["on", "off", "toggle", "statusRequest", "pct", "ani",
                    # Used from SetExtensions.pm
                    "blink", "on-for-timer", "on-till", "off-for-timer", "off-till", "intervals"],
  "EnvSensor"   => undef
);

# Hashtable for automatic device type assignment
# Format:
# "MessageGroupName:MessageName" => "Auto Device Type"
my %auto_devtype = (
  "Weather:Temperature"                   => "EnvSensor",
  "Weather:HumidityTemperature"           => "EnvSensor",
  "Weather:BarometricPressureTemperature" => "EnvSensor",
  "Environment:Brightness"                => "EnvSensor",
  "Environment:Distance"                  => "EnvSensor",
  "PowerSwitch:SwitchState"               => "PowerSwitch",
  "Dimmer:Brightness"                     => "Dimmer"
);

sub SHC_Dev_Parse($$);

sub SHC_Dev_Initialize($)
{
  my ($hash) = @_;

  $hash->{Match}    = "^Packet Data: SenderID=[1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6]";
  $hash->{SetFn}    = "SHC_Dev_Set";
  $hash->{DefFn}    = "SHC_Dev_Define";
  $hash->{UndefFn}  = "SHC_Dev_Undef";
  $hash->{ParseFn}  = "SHC_Dev_Parse";
  $hash->{AttrList} = "IODev" 
                       ." readonly:1"
                       ." forceOn:1"
                       ." $readingFnAttributes";
}

sub SHC_Dev_Define($$)
{
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if (@a < 3 || @a > 4) {
    my $msg = "wrong syntax: define <name> SHC_Dev <SenderID> [<AesKey>] ";
    Log3 undef, 2, $msg;
    return $msg;
  }

  # Correct SenderID for SHC devices is from 1 - 4096 (leading zeros allowed)
  $a[2] =~ m/^([1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6])$/i;
  return "$a[2] is not a valid SHC_Dev SenderID" if (!defined($1));

  my $aeskey;

  if (@a == 3) {
    $aeskey = 0;
  } else {
    return "$a[3] is not a valid SHC_Dev AesKey" if ($a[3] lt 0 || $a[3] gt 15);
    $aeskey = $a[3];
  }

  my $name = $a[0];
  my $addr = $a[2];

  return "SHC_Dev device $addr already used for $modules{SHC_Dev}{defptr}{$addr}->{NAME}." if ($modules{SHC_Dev}{defptr}{$addr}
    && $modules{SHC_Dev}{defptr}{$addr}->{NAME} ne $name);

  $hash->{addr}   = $addr;
  $hash->{aeskey} = $aeskey;

  $modules{SHC_Dev}{defptr}{$addr} = $hash;

  AssignIoPort($hash);
  if (defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device";
  }

  return undef;
}

#####################################
sub SHC_Dev_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete($modules{SHC_Dev}{defptr}{$addr});

  return undef;
}

#####################################

sub SHC_Dev_Parse($$)
{
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};

  if (!$parser->parse($msg)) {
    Log3 $hash, 4, "SHC_TEMP: parser error: $msg";
    return "";
  }

  my $msgtypename  = $parser->getMessageTypeName();
  my $msggroupname = $parser->getMessageGroupName();
  my $msgname      = $parser->getMessageName();
  my $raddr        = $parser->getSenderID();
  my $rhash        = $modules{SHC_Dev}{defptr}{$raddr};
  my $rname        = $rhash ? $rhash->{NAME} : $raddr;

  if (!$modules{SHC_Dev}{defptr}{$raddr}) {
    Log3 $name, 3, "SHC_TEMP: Unknown device $rname, please define it";
    return "UNDEFINED SHC_Dev_$rname SHC_Dev $raddr";
  }

  if (($msgtypename ne "Status") && ($msgtypename ne "AckStatus")) {
    Log3 $name, 3, "$rname: Ignoring MessageType $msgtypename";
    return "";
  }

  Log3 $name, 4, "$rname: Msg: $msg";
  Log3 $name, 4, "$rname: MsgType: $msgtypename, MsgGroupName: $msggroupname, MsgName: $msgname";

  my @list;
  push(@list, $rname);
  $rhash->{SHC_Dev_lastRcv} = TimeNow();
  $rhash->{SHC_Dev_pktcnt}  = $parser->getPacketCounter();
  $rhash->{SHC_Dev_msgdata} = $parser->getMessageData();
  $rhash->{SHC_Dev_msgtype} = "$msggroupname : $msgname : $msgtypename";

  my $readonly = AttrVal($rname, "readonly", "0");

  readingsBeginUpdate($rhash);

  given ($msggroupname) {
    when ('Generic') {
      given ($msgname) {
        when ('BatteryStatus') {
          readingsBulkUpdate($rhash, "battery", $parser->getField("Percentage"));
        }
        when ('Version') {
          my $major = $parser->getField("Major");
          my $minor = $parser->getField("Minor");
          my $patch = $parser->getField("Patch");
          my $vhash = $parser->getField("Hash");

          readingsBulkUpdate($rhash, "version", "$major.$minor.$patch-$vhash");
        }
      }
    }
    when ('Weather') {
      given ($msgname) {
        when ('Temperature') {
          my $tmp = $parser->getField("Temperature") / 100;    # parser returns centigrade

          readingsBulkUpdate($rhash, "temperature", $tmp);
        }
        when ('HumidityTemperature') {
          my $hum = $parser->getField("Humidity") / 10;        # parser returns 1/10 percent
          my $tmp = $parser->getField("Temperature") / 100;    # parser returns centigrade

          readingsBulkUpdate($rhash, "humidity",    $hum);
          readingsBulkUpdate($rhash, "temperature", $tmp);
        }
        when ('BarometricPressureTemperature') {
          my $bar = $parser->getField("BarometricPressure") / 100;    # parser returns pascal, use hPa
          my $tmp = $parser->getField("Temperature") / 100;           # parser returns centigrade

          # DEBUG for WORKAROUND
          # Log3 $name, 2, "$rname: HELP HELP why am I here, starting to add barometric pressure";
          readingsBulkUpdate($rhash, "barometric_pressure", $bar);
          readingsBulkUpdate($rhash, "temperature",         $tmp);

          # DEBUG for WORKAROUND
          # Log3 $name, 2, "$rname: HELP HELP am I still here";
        }
      }
    }
    when ('Environment') {
      given ($msgname) {
        when ('Brightness') {
          my $brt = $parser->getField("Brightness");
          readingsBulkUpdate($rhash, "brightness", $brt);
        }
        when ('Distance') {
          my $brt = $parser->getField("Distance");
          readingsBulkUpdate($rhash, "distance", $brt);
        }
      }
    }
    when ('PowerSwitch') {
      given ($msgname) {
        when ('SwitchState') {
          my $on      = $parser->getField("On");
          my $timeout = $parser->getField("TimeoutSec");

          readingsBulkUpdate($rhash, "on",      $on);
          readingsBulkUpdate($rhash, "timeout", $timeout);
        }
      }
    }
    when ('Dimmer') {
      given ($msgname) {
        when ('Brightness') {
          my $brightness = $parser->getField("Brightness");
          my $on = $brightness == 0 ? 0 : 1;

          readingsBulkUpdate($rhash, "on",         $on);
          readingsBulkUpdate($rhash, "brightness", $brightness);
        }
      }
    }
  }

  # Autoassign device type
  if ((!defined($rhash->{devtype})) && (defined($auto_devtype{"$msggroupname:$msgname"}))) {
    $rhash->{devtype} = $auto_devtype{"$msggroupname:$msgname"};
    Log3 $name, 3, "$rname: Autoassign device type = " . $rhash->{devtype};
  }

  # If the devtype is defined add, if not already done, the according webCmds and devStateIcons
  if (defined($rhash->{devtype})) {
    if (!defined($attr{$rname}{devStateIcon}) && defined($dev_state_icons{$rhash->{devtype}})) {
      $attr{$rname}{devStateIcon} = $dev_state_icons{$rhash->{devtype}};
    }
    if (!defined($attr{$rname}{webCmd}) && defined($web_cmds{$rhash->{devtype}})) {
      $attr{$rname}{webCmd} = $web_cmds{$rhash->{devtype}};
    }
  }

  # TODO
  # WORKAROUND
  #
  # After a fhem server restart it happens that a "barometric_pressure" reading gets added even if no
  # BarometricPressureTemperature message was received. A closer look showed that the only code sequence
  # that adds the baro reading is never executed, the reading still occurs.

  if ((defined($rhash->{READINGS}{barometric_pressure}{VAL}))
    && $rhash->{READINGS}{barometric_pressure}{VAL} == 0)
  {
    Log3 $name, 3, "$rname: WORKAROUND barometric_pressure defined, but value is invalid. Will be removed";
    delete ($rhash->{READINGS}{barometric_pressure})
  }

  # Assemble state string according to %dev_state_format
  if (defined($rhash->{devtype}) && defined($dev_state_format{$rhash->{devtype}})) {
    my $state_format_arr = $dev_state_format{$rhash->{devtype}};

    # Iterate over state_format array, if readings are available append it to the state string
    my $state_str = "";
    for (my $i = 0 ; $i < @$state_format_arr ; $i = $i + 2) {
      if (defined($rhash->{READINGS}{$state_format_arr->[$i]}{VAL})) {
        my $val = $rhash->{READINGS}{$state_format_arr->[$i]}{VAL};

        # "on" reading requires a special treatment because 0 translates to off, 1 translates to on
        if ($state_format_arr->[$i] eq "on") {
          $state_str .= $val == 0 ? "off " : "on ";
        } else {
          $state_str .= $state_format_arr->[$i + 1] . $val . " ";
        }

        # DEBUG
        # Log3 $name, 4, "$rname: $i " . $state_format_arr->[$i] . " " . $state_format_arr->[$i + 1] . " " . $val;
      }
    }

    readingsBulkUpdate($rhash, "state", $state_str);
  }

  readingsEndUpdate($rhash, 1);    # Do triggers to update log file
  return @list;
}

#####################################
sub SHC_Dev_Set($@)
{
  my ($hash, $name, @aa) = @_;
  my $cnt = @aa;

  my $cmd   = $aa[0];
  my $arg   = $aa[1];
  my $arg2  = $aa[2];
  my $arg3  = $aa[3];
  my $arg4  = $aa[4];

  return "\"set $name\" needs at least one parameter" if ($cnt < 1);

  if ($cmd eq "devtype") {
    if (exists($sets{$arg})) {
      $hash->{devtype} = $arg;
      Log3 $name, 3, "$name: devtype set to \"$arg\"";
      return undef;
    } else {
      return "devtype \"$arg\" not supported. Currently supported device types: " . join(", ", sort keys %sets);
    }
  }

  if (!defined($hash->{devtype})) {
    return "\"devtype\" not yet specifed. Currently supported device types: " . join(", ", sort keys %sets);
  }

  if (!defined($sets{$hash->{devtype}})) {
    return "No set commands supported for device type: " . $hash->{devtype};
  }

  # TODO:
  # Currently the commands for every device type are defined in %sets but not used to verify the commands. Instead
  # the SetExtension.pm modulesis used for this purpose.
  # For more sophisticated device types this has to be revisited

  my $readonly = AttrVal($name, "readonly", "0");

  given ($hash->{devtype}) {
    when ('PowerSwitch') {
      my $list = "statusRequest:noArg";
      $list .= " off:noArg on:noArg toggle:noArg" if (!$readonly);

      # Timeout functionality for SHC_Dev is not implemented, because FHEMs internal notification system
      # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

      if ($cmd eq 'toggle') {
        $cmd = ReadingsVal($name, "state", "on") eq "off" ? "on" : "off";
      }

      if (!$readonly && $cmd eq 'off') {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("PowerSwitch", "SwitchState", "SetGet");
        $parser->setField("PowerSwitch", "SwitchState", "TimeoutSec", 0);
        $parser->setField("PowerSwitch", "SwitchState", "On",         0);
        SHC_Dev_Send($hash);
      } elsif (!$readonly && $cmd eq 'on') {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("PowerSwitch", "SwitchState", "SetGet");
        $parser->setField("PowerSwitch", "SwitchState", "TimeoutSec", 0);
        $parser->setField("PowerSwitch", "SwitchState", "On",         1);
        SHC_Dev_Send($hash);
      } elsif ($cmd eq 'statusRequest') {
        $parser->initPacket("PowerSwitch", "SwitchState", "Get");
        SHC_Dev_Send($hash);
      } else {
        return SetExtensions($hash, $list, $name, @aa);
      }
    }
    when ('Dimmer') {
      my $list = "statusRequest:noArg";
      $list .= " ani pct:slider,0,1,100 off:noArg on:noArg" if (!$readonly);

      # Timeout functionality for SHC_Dev is not implemented, because FHEMs internal notification system
      # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

      if ($cmd eq 'toggle') {
        $cmd = ReadingsVal($name, "state", "on") eq "off" ? "on" : "off";
      }

      if (!$readonly && $cmd eq 'off') {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
        $parser->setField("Dimmer", "Brightness", "Brightness", 0);
        SHC_Dev_Send($hash);
      } elsif (!$readonly && $cmd eq 'on') {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
        $parser->setField("Dimmer", "Brightness", "Brightness", 100);
        SHC_Dev_Send($hash);
      } elsif (!$readonly && $cmd eq 'pct') {
        my $brightness = $arg;

        #DEBUG
        Log3 $name, 3, "$name: Args: $arg, $arg2, $arg3, $brightness";

        readingsSingleUpdate($hash, "state", "set-pct:$brightness", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
        $parser->setField("Dimmer", "Brightness", "Brightness", $brightness);
        SHC_Dev_Send($hash);
      } elsif (!$readonly && $cmd eq 'ani') {

        #TODO Verify argument values
        my $brightness = $arg;

        #DEBUG
        Log3 $name, 3, "$name: ani args: $arg, $arg2, $arg3, $arg4, $brightness";

        readingsSingleUpdate($hash, "state", "set-ani", 1);
        $parser->initPacket("Dimmer", "Animation", "SetGet");
        $parser->setField("Dimmer", "Animation", "AnimationMode",   $arg);
        $parser->setField("Dimmer", "Animation", "TimeoutSec",      $arg2);
        $parser->setField("Dimmer", "Animation", "StartBrightness", $arg3);
        $parser->setField("Dimmer", "Animation", "EndBrightness",   $arg4);
        SHC_Dev_Send($hash);
      } elsif ($cmd eq 'statusRequest') {
        $parser->initPacket("Dimmer", "Brightness", "Get");
        SHC_Dev_Send($hash);
      } else {
        return SetExtensions($hash, $list, $name, @aa);
      }
    }
  }

  return undef;
}

sub SHC_Dev_Send($)
{
  my ($hash) = @_;
  my $name = $hash->{NAME};

  $hash->{SHC_Dev_lastSend} = TimeNow();

  my $msg = $parser->getSendString($hash->{addr}, $hash->{aeskey});

  Log3 $name, 3, "$name: Sending $msg";

  IOWrite($hash, $msg);
}

1;

=pod
=begin html

<a name="SHC_Dev"></a>
<h3>SHC_Dev</h3>
<ul>

  <tr><td>
  The SHC_Dev A secure and extendable Open Source home automation system.<br><br>
  
  More info can be found in <a href="https://www.smarthomatic.org">Smarthomatic Website</a><br><br>

  <a name="SHC_Dev_Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; SHC_Dev &lt;SenderID&gt; [&lt;AesKey&gt;]</code> <br>
    <br>
    <li><code>&lt;SenderID<li><code>&lt; is a number ranging from 0 .. 4095 to identify the SHC_Dev device.
    <li>The optional <code>&lt;AesKey<li><code>&lt; is a number ranging from 0 .. 15 to select an encryption key.
    It is required for the basestation to communicate with remote devides
    The default value is 0.<br><br>
    Note: devices are autocreated on reception of the first message.<br>
  </ul>
  <br>

  <a name="SHC_Dev_Set"></a>
  <b>Set</b>
  <ul>
    <li>devtype</b>
    The device type determines the command set, default web commands and the default devStateicon</b
    Currently supported are: EnvSensor, Dimmer, PowerSwitch></li>
    <li>on (Dimmer, PowerSwitch)</li>
    <li>off (Dimmer, PowerSwitch)</li>
    <li>pct <0..100>  Sets the brightness in percent (Dimmer)</li>
    <li>ani <AnimationMode> <TimeoutSec> <StartBrightness> <EndBrightness> (Dimmer)</b>
    Details in <a href="http://www.smarthomatic.org/basics/message_catalog.html#Dimmer_Animation">Smarthomatic Website</a></li>
    <li>statusRequest (Dimmer, PowerSwitch)</li>
    <li><a href="#setExtensions"> set extensions</a> (Dimmer, PowerSwitch) are supported.</li>
  </ul><br>

  <a name="SHC_Dev_Get"></a>
  <b>Get</b>
  <ul>
    <li>N/A</li>
  </ul><br>

  <a name="SHC_Dev_Readings"></a>
  <b>Readings</b>
  <ul>
    <li>N/A</li>
  </ul><br>

  <a name="SHC_Dev_Attr"></a>
  <b>Attributes</b>
  <ul>
    <li>readonly<br>
    if set to a value != 0 all switching commands (on, off, toggle, ...) will be disabled.</li>
    <li>forceOn<br>
    try to switch on the device whenever an off status is received.</li>
  </ul><br>
</ul>

=end html
=cut
