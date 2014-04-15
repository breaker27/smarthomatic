# Copied from 36_PCA301.pm and adapted
# $Id: 36_SHC_TEMP.pm 3934 2013-09-21 09:21:39Z justme1968 $
#
# TODO:

package main;

use strict;
use feature qw(switch);
use warnings;
use SetExtensions;

use SHC_parser;

my $parser = new SHC_parser();

sub SHC_TEMP_Parse($$);

sub
SHC_TEMP_Initialize($)
{
  my ($hash) = @_;

  $hash->{Match}     = "^Packet Data: SenderID=[1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6]";
  $hash->{SetFn}     = "SHC_TEMP_Set";
  $hash->{DefFn}     = "SHC_TEMP_Define";
  $hash->{UndefFn}   = "SHC_TEMP_Undef";
  $hash->{ParseFn}   = "SHC_TEMP_Parse";
  $hash->{AttrList}  = "IODev"
                       ." readonly:1"
                       ." forceOn:1"
                       ." $readingFnAttributes";
}

sub
SHC_TEMP_Define($$)
{
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if(@a < 3 || @a > 4) {
    my $msg = "wrong syntax: define <name> SHC_TEMP <SenderID> [<AesKey>] ";
    Log3 undef, 2, $msg;
    return $msg;
  }
  # Correct SenderID for SHC devices is from 1 - 4096 (leading zeros allowed)
  $a[2] =~ m/^([1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6])$/i;
  return "$a[2] is not a valid SHC_TEMP SenderID" if( !defined($1) );

  my $aeskey;

  if(@a eq 3){
    $aeskey = 0;
  } else {
    return "$a[3] is not a valid SHC_TEMP AesKey" if($a[3] lt 0 || $a[3] gt 15);
    $aeskey = $a[3]
  }

  my $name = $a[0];
  my $addr = $a[2];

  return "SHC_TEMP device $addr already used for $modules{SHC_TEMP}{defptr}{$addr}->{NAME}." if( $modules{SHC_TEMP}{defptr}{$addr}
                                                                                             && $modules{SHC_TEMP}{defptr}{$addr}->{NAME} ne $name );

  $hash->{addr} = $addr;
  $hash->{aeskey} = $aeskey;

  $modules{SHC_TEMP}{defptr}{$addr} = $hash;

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
SHC_TEMP_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};
  my $addr = $hash->{addr};

  delete( $modules{SHC_TEMP}{defptr}{$addr} );

  return undef;
}

#####################################

sub
SHC_TEMP_Parse($$)
{
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};
  my ($senderid, $pktcnt, $msgtypename, $msggroupname, $msgname, $msgdata);


  if( !$parser->parse($msg) ) {
    Log3 $hash, 4, "SHC_TEMP  ($msg) data error";
    return "";
  }

  $senderid = $parser->getSenderID();
  $pktcnt = $parser->getPacketCounter();
  $msgtypename = $parser->getMessageTypeName();
  $msggroupname = $parser->getMessageGroupName();
  $msgname = $parser->getMessageName();
  $msgdata = $parser->getMessageData();

  my $raddr = $senderid;
  my $rhash = $modules{SHC_TEMP}{defptr}{$raddr};
  my $rname = $rhash?$rhash->{NAME}:$raddr;

  if( !$modules{SHC_TEMP}{defptr}{$raddr} ) {
     Log3 $name, 3, "SHC_TEMP Unknown device $rname, please define it";

     return "UNDEFINED SHC_TEMP_$rname SHC_TEMP $raddr";
  }

  my @list;
  push(@list, $rname);
  $rhash->{SHC_TEMP_lastRcv} = TimeNow();

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
          # power switch. Define device type and addd according web commands
          $rhash->{devtype} = "PowerSwitch" if ( !defined($rhash->{devtype}) );
          $attr{$rname}{devStateIcon} = 'on:on:toggle off:off:toggle set.*:light_question:off' if( !defined( $attr{$rname}{devStateIcon} ) );
          $attr{$rname}{webCmd} = 'on:off:toggle:statusRequest' if( !defined( $attr{$rname}{webCmd} ) );
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
SHC_TEMP_Set($@)
{
  my ($hash, $name, @aa) = @_;

  my $cnt = @aa;

  return "\"set $name\" needs at least one parameter" if($cnt < 1);

  return undef if ( !defined( $hash->{devtype}) || $hash->{devtype} eq "EnvSensor" );

  my $cmd = $aa[0];
  my $arg = $aa[1];
  my $arg2 = $aa[2];
  my $arg3 = $aa[3];

  my $readonly = AttrVal($name, "readonly", "0" );

  my $list = "statusRequest:noArg";
  $list .= " off:noArg on:noArg toggle:noArg" if( !$readonly );

  # Timeout functionality for SHC_TEMP is not implemented, because FHEMs internal notification system
  # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

  $parser->initPacket("PowerSwitch", "SwitchState", "Set");
  $parser->setField("PowerSwitch", "SwitchState", "TimeoutSec", 0);

  if( $cmd eq 'toggle' ) {
    $cmd = ReadingsVal($name,"state","on") eq "off" ? "on" :"off";
  }

  if( !$readonly && $cmd eq 'off' ) {
    readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    $parser->setField("PowerSwitch", "SwitchState", "On", 0);
    SHC_TEMP_Send($hash);
  } elsif( !$readonly && $cmd eq 'on' ) {
    readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    $parser->setField("PowerSwitch", "SwitchState", "On", 1);
    SHC_TEMP_Send($hash);
  } elsif( $cmd eq 'statusRequest' ) {
    # TODO implement with Get command
    # readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    # SHC_TEMP_Send( $hash, "00", "0000" );
  } else {
    return SetExtensions($hash, $list, $name, @aa);
  }

  return undef;
}

sub
SHC_TEMP_Send($)
{
  my ($hash) = @_;
  my $name = $hash->{NAME};

  $hash->{SHC_TEMP_lastSend} = TimeNow();

  my $msg = $parser->getSendString( $hash->{addr}, $hash->{aeskey} );

  # WORKAROUND for bug in SHC_parser.pm
  # $msg = substr($msg, 0, 17);
  $msg = "$msg\r";

  Log3 $name, 3, "$name: Sending $msg";

  IOWrite( $hash, $msg );
}

1;

=pod
=begin html

<a name="SHC_TEMP"></a>
<h3>SHC_TEMP</h3>
<ul>

  <tr><td>
  The SHC_TEMP A secure and extendable Open Source home automation system.<br><br>
  
  More info can be found in <a href="https://www.smarthomatic.org">Smarthomatic Website</a><br><br>

  <a name="SHC_TEMP_Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; SHC_TEMP &lt;SenderID&gt; [&lt;AesKey&gt;]</code> <br>
    <br>
    <li><code>&lt;SenderID<li><code>&lt; is a number ranging from 0 .. 4095 to identify the SHC_TEMP device.
    <li>The optional <code>&lt;AesKey<li><code>&lt; is a number ranging from 0 .. 15 to select an encryption key.
    It is required for the basestation to communicate with remote devides
    The default value is 0.<br><br>
    Note: devices are autocreated on reception of the first message.<br>
  </ul>
  <br>

  <a name="SHC_TEMP_Set"></a>
  <b>Set</b>
  <ul>
    <li>on</li>
    <li>off</li>
    <li>statusRequest</li>
    <li><a href="#setExtensions"> set extensions</a> are supported.</li>
  </ul><br>

  <a name="SHC_TEMP_Get"></a>
  <b>Get</b>
  <ul>
    <li>N/A</li>
  </ul><br>

  <a name="SHC_TEMP_Readings"></a>
  <b>Readings</b>
  <ul>
    <li>N/A</li>
  </ul><br>

  <a name="SHC_TEMP_Attr"></a>
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
