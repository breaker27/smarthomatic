# Copied from 36_PCA301.pm and adapted
# $Id: 36_SHC_TEMP.pm 3934 2013-09-21 09:21:39Z justme1968 $
#
# TODO:

package main;

use strict;
use warnings;
use SetExtensions;

sub SHC_TEMP_Parse($$);

sub
SHC_TEMP_Initialize($)
{
  my ($hash) = @_;

  $hash->{Match}     = "^Packet Data: SenderID=[0-9]";
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

  $a[2] =~ m/^([0-9][0-9])$/i;	# TODO Whats the appropriate range for SHC
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
  my ($id, $pktcnt, $msgtype, $msggroupid, $msgid, $msgdata);

  # NEW smarthomatic v0.5.0
  #
  # SHC MESSAGES (Details in http://www.smarthomatic.org/basics/message_catalog.html):
  # Generic (MsgGroup=0):
  #   Version(1):
  #     Packet Data: SenderID=31;PacketCounter=809;MessageType=8;MessageGroupID=0;MessageID=1;MessageData=00000000000000000000000000000000000000000000;Major=0;Minor=0;Patch=0;Hash=00000000;
  #   Battery(5):
  #     Packet Data: SenderID=31;PacketCounter=102;MessageType=8;MessageGroupID=0;MessageID=5;MessageData=c20000000004;Percentage=97;
  # EnvSensor (MsgGroup=10):
  #   TempHumBriStatus(1):
  #     Packet Data: SenderID=31;PacketCounter=677;MessageType=8;MessageGroupID=10;MessageID=1;MessageData=0960001f0001;Temperature=24.00;Humidity=0.0;Brightness=62;
  # PowerSwitch (MsgGroup=20):
  #   SwitchState(1)
  #     Packet Data: SenderID=40;PacketCounter=6533;MessageType=8;MessageGroupID=20;MessageID=1;MessageData=ac8c80000000;On=1;TimeoutSec=22809;
  if ($msg =~ /^Packet Data: SenderID=(\d*);PacketCounter=(\d*);MessageType=(\d*);MessageGroupID=(\d*);MessageID=(\d*);MessageData=([0-9a-f]*)/) {
    $id = $1;
    $pktcnt = $2;
    $msgtype = $3;
    $msggroupid = $4;
    $msgid = $5;
    $msgdata = $6;

  } else {
    Log3 $hash, 4, "SHC_TEMP  ($msg) data error";
    return "";
  }

  my $raddr = $id;
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
  readingsBulkUpdate($rhash, "id", $id);
  readingsBulkUpdate($rhash, "pktcnt", $pktcnt);
  readingsBulkUpdate($rhash, "msgtype", $msgtype);
  readingsBulkUpdate($rhash, "msggroupid", $msggroupid);
  readingsBulkUpdate($rhash, "msgid", $msgid);
  readingsBulkUpdate($rhash, "msgdata", $msgdata);

  if ($msg =~ /.*;Major=(\d*);Minor=(\d*);Patch=(\d*);Hash=([0-9a-zA-Z]*);/)
  {
    # Generic (MsgGroup=0): Version(1):
    readingsBulkUpdate($rhash, "version_major", $1);
    readingsBulkUpdate($rhash, "version_minor", $2);
    readingsBulkUpdate($rhash, "version_patch", $3);
    readingsBulkUpdate($rhash, "version_hash", $4);
  }
  elsif ($msg =~ /.*;Percentage=(\d*);/)
  {
    # Generic (MsgGroup=0): Battery(5):
    readingsBulkUpdate($rhash, "battery", $1);
  }
  elsif ($msg =~ /.*;Temperature=([0-9\.\-]*);Humidity=([0-9\.]*);Brightness=([0-9\.]*);/)
  {
    # EnvSensor (MsgGroup=10): TempHumBriStatus(1):
    my ($tmp, $hum, $brt);  # temperature, humidity, brightness

    $tmp = $1;
    $hum = $2;
    $brt = $3;

    readingsBulkUpdate($rhash, "state", "T: $tmp  H: $hum  B:$brt");
    readingsBulkUpdate($rhash, "temperature", $tmp);
    readingsBulkUpdate($rhash, "humidity", $hum);
    readingsBulkUpdate($rhash, "brightness", $brt);
  }
  elsif ($msg =~ /.*;On=(\d*);TimeoutSec=(\d*);/)
  {
    # PowerSwitch (MsgGroup=20): SwitchState(1)
    my ($on, $timeout, $state); # power switch status

    $on = $1;
    $timeout = $2;
    $state = $on==0?"off":"on";

    readingsBulkUpdate($rhash, "state", $state);
    readingsBulkUpdate($rhash, "on", $on);
    readingsBulkUpdate($rhash, "timeout", $timeout);
  }

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

  my $cmd = $aa[0];
  my $arg = $aa[1];
  my $arg2 = $aa[2];
  my $arg3 = $aa[3];

  my $readonly = AttrVal($name, "readonly", "0" );

  my $list = "";
  $list .= " off:noArg on:noArg toggle:noArg" if( !$readonly );

  # Timeout functionality for SHC_TEMP is not implemented, because FHEMs internal notification system 
  # is able to do this as well. Even more it supports intervals, off-for-timer, off-till ...

  if( $cmd eq 'toggle' ) {
    $cmd = ReadingsVal($name,"state","on") eq "off" ? "on" :"off";
  }

  if( !$readonly && $cmd eq 'off' ) {
    readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    SHC_TEMP_Send( $hash, "01", "0000" );
  } elsif( !$readonly && $cmd eq 'on' ) {
    readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    SHC_TEMP_Send( $hash, "01", "8000" );
  } elsif( $cmd eq 'statusRequest' ) {
    readingsSingleUpdate($hash, "state", "set-$cmd", 1);
    SHC_TEMP_Send( $hash, "08", "" );
  } else {
    return SetExtensions($hash, $list, $name, @aa);
  }

  return undef;
}

sub
SHC_TEMP_Send($$@)
{
  my ($hash, $cmd, $data) = @_;

  # sKK{T}{X}{D}...Use AES key KK to send a packet with MessageType T, followed
  #                by all necessary extension header fields and message data.
  #                Fields are: ReceiverID (RRRR), MessageGroup (GG), MessageID (MM)
  #                AckSenderID (SSSS), AckPacketCounter (PPPPPP), Error (EE).
  #                MessageData (DD) can be 0..17 bytes with bits moved to the left.
  #                End data with ENTER. SenderID, PacketCounter and CRC are automatically added.
  # sKK00RRRRGGMMDD...........Get
  # sKK01RRRRGGMMDD...........Set
  # sKK02RRRRGGMMDD...........SetGet
  # sKK08GGMMDD...............Status
  # sKK09SSSSPPPPPPEE.........Ack
  # sKK0ASSSSPPPPPPEEGGMMDD...AckStatus

  my $msggrp = "14";         # msggrp = 20 convert to hex = 14
  my $msgid = "01";

  $hash->{SHC_TEMP_lastSend} = TimeNow();

  my $msg = sprintf( "s%02x%s%04x%s%s%s\r", $hash->{aeskey}, $cmd, $hash->{addr}, $msggrp, $msgid, $data );

  # DEBUG
  Log3 "SHC_TEMP_Send", 1, "SHC_TEMP_SEND: $msg";

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
    <li>statusRequest<br>
      Not supported yes.</li>
    <li><a href="#setExtensions"> set extensions</a> are supported.</li>
  </ul><br>

  <a name="SHC_TEMP_Get"></a>
  <b>Get</b>
  <ul>
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
