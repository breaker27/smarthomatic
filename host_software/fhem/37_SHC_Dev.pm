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

sub SHC_Dev_Parse($$);

sub
SHC_Dev_Initialize($)
{
  my ($hash) = @_;

  $hash->{Match}     = "^Packet Data: SenderID=[1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6]";
  $hash->{SetFn}     = "SHC_Dev_Set";
  $hash->{DefFn}     = "SHC_Dev_Define";
  $hash->{UndefFn}   = "SHC_Dev_Undef";
  $hash->{ParseFn}   = "SHC_Dev_Parse";
  $hash->{AttrList}  = "IODev"
                       ." readonly:1"
                       ." forceOn:1"
                       ." $readingFnAttributes";
}

sub
SHC_Dev_Define($$)
{
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a < 3 || @a > 4) {
    my $msg = "wrong syntax: define <name> SHC_Dev <SenderID> [<AesKey>] ";
    Log3 undef, 2, $msg;
    return $msg;
  }
  # Correct SenderID for SHC devices is from 1 - 4096 (leading zeros allowed)
  $a[2] =~ m/^([1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6])$/i;
  return "$a[2] is not a valid SHC_Dev SenderID" if( !defined($1) );

  my $aeskey;

  if(@a eq 3){
    $aeskey = 0;
  } else {
    return "$a[3] is not a valid SHC_Dev AesKey" if($a[3] lt 0 || $a[3] gt 15);
    $aeskey = $a[3]
  }

  my $name = $a[0];
  my $addr = $a[2];

  return "SHC_Dev device $addr already used for $modules{SHC_Dev}{defptr}{$addr}->{NAME}." if( $modules{SHC_Dev}{defptr}{$addr}
                                                                                             && $modules{SHC_Dev}{defptr}{$addr}->{NAME} ne $name );

  $hash->{addr} = $addr;
  $hash->{aeskey} = $aeskey;

  $modules{SHC_Dev}{defptr}{$addr} = $hash;

  AssignIoPort($hash);
  if(defined($hash->{IODev}->{NAME})) {
    Log3 $name, 3, "$name: I/O device is " . $hash->{IODev}->{NAME};
  } else {
    Log3 $name, 1, "$name: no I/O device";
  }

  return undef;
}

#####################################
sub
SHC_Dev_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete( $modules{SHC_Dev}{defptr}{$addr} );

  return undef;
}

#####################################

sub
SHC_Dev_Parse($$)
{
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};
  my ($senderid, $pktcnt, $msgtypename, $msggroupname, $msgname, $msgdata);


  if( !$parser->parse($msg) ) {
    Log3 $hash, 4, "$name: parser error: $msg";
    return "";
  }

  $senderid = $parser->getSenderID();
  $pktcnt = $parser->getPacketCounter();
  $msgtypename = $parser->getMessageTypeName();
  $msggroupname = $parser->getMessageGroupName();
  $msgname = $parser->getMessageName();
  $msgdata = $parser->getMessageData();
  
  if (($msgtypename ne "Status") && ($msgtypename ne "AckStatus"))
  {
	Log3 $name, 3, "$name: Ignoring MessageType $msgtypename";
	return "";
  }
  
  Log3 $name, 4, "$name: MessageType is $msgtypename";
  
  my $raddr = $senderid;
  my $rhash = $modules{SHC_Dev}{defptr}{$raddr};
  my $rname = $rhash?$rhash->{NAME}:$raddr;

  if( !$modules{SHC_Dev}{defptr}{$raddr} ) {
     Log3 $name, 3, "$name: Unknown device $rname, please define it";

     return "UNDEFINED SHC_Dev_$rname SHC_Dev $raddr";
  }

  my @list;
  push(@list, $rname);
  $rhash->{SHC_Dev_lastRcv} = TimeNow();

  my $readonly = AttrVal($rname, "readonly", "0" );

  readingsBeginUpdate($rhash);
  readingsBulkUpdate($rhash, "senderid", $senderid);
  readingsBulkUpdate($rhash, "pktcnt", $pktcnt);
  readingsBulkUpdate($rhash, "msgtypename", $msgtypename);
  readingsBulkUpdate($rhash, "msggroupname", $msggroupname);
  readingsBulkUpdate($rhash, "msgname", $msgname);
  readingsBulkUpdate($rhash, "msgdata", $msgdata);

  given($msggroupname)
  {
    when('Generic')
    {
      given($msgname)
      {
        when('BatteryStatus')
        {
          readingsBulkUpdate($rhash, "battery", $parser->getField("Percentage"));
        }
        when('Version')
        {
          my $major = $parser->getField("Major");
          my $minor = $parser->getField("Minor");
          my $patch = $parser->getField("Patch");
          my $vhash = $parser->getField("Hash");

          readingsBulkUpdate($rhash, "version", "$major.$minor.$patch-$vhash");
        }
      }
    }
    when('EnvSensor')
    {
      given($msgname)
      {
        when('TempHumBriStatus')
        {
          my $tmp = $parser->getField("Temperature") / 100; # parser returns centigrade
          my $hum = $parser->getField("Humidity") / 10;     # parser returns 1/10 percent
          my $brt = $parser->getField("Brightness");

          readingsBulkUpdate($rhash, "state", "T: $tmp  H: $hum  B:$brt");
          readingsBulkUpdate($rhash, "temperature", $tmp);
          readingsBulkUpdate($rhash, "humidity", $hum);
          readingsBulkUpdate($rhash, "brightness", $brt);
          # After receiving this message we know for the first time that we are a 
          # enviroment sonsor, so lets define our device type
          $rhash->{devtype} = "EnvSensor" if ( !defined($rhash->{devtype} ) );
        }
      }
    }
    when('PowerSwitch')
    {
      given($msgname)
      {
        when('SwitchState')
        {
          my $on = $parser->getField("On");
          my $timeout = $parser->getField("TimeoutSec");
          my $state = $on==0?"off":"on";

          readingsBulkUpdate($rhash, "state", $state);
          readingsBulkUpdate($rhash, "on", $on);
          readingsBulkUpdate($rhash, "timeout", $timeout);

          # After receiving this message we know for the first time that we are a 
          # power switch. Define device type and add according web commands
          $rhash->{devtype} = "PowerSwitch" if ( !defined($rhash->{devtype}) );
          $attr{$rname}{devStateIcon} = 'on:on:toggle off:off:toggle set.*:light_question:off' if( !defined( $attr{$rname}{devStateIcon} ) );
          $attr{$rname}{webCmd} = 'on:off:toggle:statusRequest' if( !defined( $attr{$rname}{webCmd} ) );
        }
      }
    }
    when('Dimmer')
    {
      given($msgname)
      {
        when('Brightness')
        {
          my $brightness = $parser->getField("Brightness");
          my $state = $brightness==0?"off":"on";

          readingsBulkUpdate($rhash, "state", $state);
          readingsBulkUpdate($rhash, "brightness", $brightness);

          # After receiving this message we know for the first time that we are a 
          # dimmer. Define device type and add according web commands
          $rhash->{devtype} = "Dimmer" if ( !defined($rhash->{devtype}) );
          $attr{$rname}{devStateIcon} = 'on:on off:off set.*:light_question:off' if( !defined( $attr{$rname}{devStateIcon} ) );
          $attr{$rname}{webCmd} = 'on:off:statusRequest' if( !defined( $attr{$rname}{webCmd} ) );
        }
      }
    }
  }

  # TODO: How to handle ACK packets?
  #   Packet Data: SenderID=40;PacketCounter=1105;MessageType=10;AckSenderID=0;AckPacketCounter=2895;Error=0;MessageGroupID=20;MessageID=1;MessageData=8000000000000000000000000000000000;On=1;TimeoutSec=0;

  readingsEndUpdate($rhash,1);    # Do triggers to update log file
  return @list;
}

#####################################
sub
SHC_Dev_Set($@)
{
  my ($hash, $name, @aa) = @_;
  my $cnt = @aa;

  return "\"set $name\" needs at least one parameter" if($cnt < 1);

  return undef if ( !defined( $hash->{devtype}) || $hash->{devtype} eq "EnvSensor" );

  my $cmd = $aa[0];
  my $arg = $aa[1];
  my $arg2 = $aa[2];
  my $arg3 = $aa[3];
  my $arg4 = $aa[4];

  my $readonly = AttrVal( $name, "readonly", "0" );


  given($hash->{devtype})
  {
    when('PowerSwitch')
    {
      my $list = "statusRequest:noArg";
      $list .= " off:noArg on:noArg toggle:noArg" if( !$readonly );

      # Timeout functionality for SHC_Dev is not implemented, because FHEMs internal notification system
      # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

      $parser->initPacket("PowerSwitch", "SwitchState", "SetGet");
      $parser->setField("PowerSwitch", "SwitchState", "TimeoutSec", 0);

      if( $cmd eq 'toggle' ) {
        $cmd = ReadingsVal($name,"state","on") eq "off" ? "on" :"off";
      }

      if( !$readonly && $cmd eq 'off' ) {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->setField("PowerSwitch", "SwitchState", "On", 0);
        SHC_Dev_Send($hash);
      } elsif( !$readonly && $cmd eq 'on' ) {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->setField("PowerSwitch", "SwitchState", "On", 1);
        SHC_Dev_Send($hash);
      } elsif( $cmd eq 'statusRequest' ) {
        # TODO implement with Get command
        # readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        # SHC_Dev_Send( $hash, "00", "0000" );
      } else {
        return SetExtensions($hash, $list, $name, @aa);
      }
    }
    when('Dimmer')
    {
      my $list = "statusRequest:noArg";
      $list .= " ani pct:slider,0,1,100 off:noArg on:noArg" if( !$readonly );

      # Timeout functionality for SHC_Dev is not implemented, because FHEMs internal notification system
      # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

      if( $cmd eq 'toggle' ) {
        $cmd = ReadingsVal($name,"state","on") eq "off" ? "on" :"off";
      }

      if( !$readonly && $cmd eq 'off' ) {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
        $parser->setField("Dimmer", "Brightness", "Brightness", 0);
        SHC_Dev_Send($hash);
      } elsif( !$readonly && $cmd eq 'on' ) {
        readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
		$parser->setField("Dimmer", "Brightness", "Brightness", 100);
        SHC_Dev_Send($hash);
      } elsif( !$readonly && $cmd eq 'pct' ) {
        my $brightness = $arg;
        #DEBUG
        Log3 $name, 3, "$name: Args: $arg, $arg2, $arg3, $brightness";

        readingsSingleUpdate($hash, "state", "set-pct:$brightness", 1);
        $parser->initPacket("Dimmer", "Brightness", "SetGet");
		$parser->setField("Dimmer", "Brightness", "Brightness", $brightness);
        SHC_Dev_Send($hash);
      } elsif( !$readonly && $cmd eq 'ani' ) {
        #TODO Verify argument values
        my $brightness = $arg;
        #DEBUG
        Log3 $name, 3, "$name: ani args: $arg, $arg2, $arg3, $arg4, $brightness";

        readingsSingleUpdate($hash, "state", "set-ani", 1);
		$parser->initPacket("Dimmer", "Animation", "SetGet");
        $parser->setField("Dimmer", "Animation", "AnimationMode", $arg);
        $parser->setField("Dimmer", "Animation", "TimeoutSec", $arg2);
        $parser->setField("Dimmer", "Animation", "StartBrightness", $arg3);
        $parser->setField("Dimmer", "Animation", "EndBrightness", $arg4);
        SHC_Dev_Send($hash);
      } elsif( $cmd eq 'statusRequest' ) {
        # TODO implement with Get command
        # readingsSingleUpdate($hash, "state", "set-$cmd", 1);
        # SHC_Dev_Send( $hash, "00", "0000" );
      } else {
        return SetExtensions($hash, $list, $name, @aa);
      }
    }
  }

  return undef;
}

sub
SHC_Dev_Send($)
{
  my ($hash) = @_;
  my $name = $hash->{NAME};

  $hash->{SHC_Dev_lastSend} = TimeNow();

  my $msg = $parser->getSendString( $hash->{addr}, $hash->{aeskey} );

  Log3 $name, 3, "$name: Sending $msg";

  IOWrite( $hash, $msg );
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
    <li>on</li>
    <li>off</li>
    <li>statusRequest</li>
    <li><a href="#setExtensions"> set extensions</a> are supported.</li>
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
