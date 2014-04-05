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

  if(@a != 3 ) {
    my $msg = "wrong syntax: define <name> SHC_TEMP <id> ";
    Log3 undef, 2, $msg;
    return $msg;
  }

  $a[2] =~ m/^([0-9][0-9])$/i;	# TODO Whats the appropriate range for SHC
  return "$a[2] is not a valid SHC_TEMP id" if( !defined($1) );

  my $name = $a[0];
  my $addr = $a[2];

  return "SHC_TEMP device $addr already used for $modules{SHC_TEMP}{defptr}{$addr}->{NAME}." if( $modules{SHC_TEMP}{defptr}{$addr}
                                                                                             && $modules{SHC_TEMP}{defptr}{$addr}->{NAME} ne $name );

  $hash->{addr} = $addr;

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
  my ($tmp, $hum, $brt, $bat);  # temperature, humidity, brightness, battery
  my ($vmajor, $vminor, $vpatch, $vhash); # version information
  
  # NEW smarthomatic v0.5.0
  # Data Message:
  #   Packet Data: SenderID=31;PacketCounter=677;MessageType=8;MessageGroupID=10;MessageID=1;MessageData=0960001f0001;Temperature=24.00;Humidity=0.0;Brightness=62;
  # Battery Status Message:
  #   Packet Data: SenderID=31;PacketCounter=102;MessageType=8;MessageGroupID=0;MessageID=5;MessageData=c20000000004;Percentage=97;
  # Version Status Message:
  #   Packet Data: SenderID=31;PacketCounter=809;MessageType=8;MessageGroupID=0;MessageID=1;MessageData=00000000000000000000000000000000000000000000;Major=0;Minor=0;Patch=0;Hash=00000000;
  
  #if ($msg =~ /^Packet Data: SenderID=(\d*);PacketCounter=(\d*);MessageType=(\d*);MessageGroupID=(\d*);MessageID=(\d*);MessageData=([0-9a-f]*);Temperature=([0-9\.\-]*);Humidity=([0-9\.]*);Brightness=([0-9\.]*);/)
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

  if ($msg =~ /.*;Temperature=([0-9\.\-]*);Humidity=([0-9\.]*);Brightness=([0-9\.]*);/) {
    $tmp = $1;
    $hum = $2;
    $brt = $3;

    readingsBulkUpdate($rhash, "state", "T: $tmp  H: $hum  B:$brt");
    readingsBulkUpdate($rhash, "temperature", $tmp);
    readingsBulkUpdate($rhash, "humidity", $hum);
    readingsBulkUpdate($rhash, "brightness", $brt);

  } elsif ($msg =~ /.*;Percentage=(\d*);/) {
    $bat = $1;

    readingsBulkUpdate($rhash, "battery", $bat);
  } elsif ($msg =~ /.*;Major=(\d*);Minor=(\d*);Patch=(\d*);Hash=([0-9a-zA-Z]*);/) {
    $vmajor = $1;
    $vminor = $2;
    $vpatch = $3;
    $vhash = $4;

    readingsBulkUpdate($rhash, "version_major", $vmajor);
    readingsBulkUpdate($rhash, "version_minor", $vminor);
    readingsBulkUpdate($rhash, "version_patch", $vpatch);
    readingsBulkUpdate($rhash, "version_hash", $vhash);
  }


  readingsEndUpdate($rhash,1);    # Do triggers to update log file
  return @list;
}

1;

=pod
=begin html

<a name="SHC_TEMP"></a>
<h3>SHC_TEMP</h3>
<ul>

  <tr><td>
  The SHC_TEMP is a RF controlled AC mains plug with integrated power meter functionality from ELV.<br><br>

  It can be integrated in to FHEM via a <a href="#JeeLink">JeeLink</a> as the IODevice.<br><br>

  The JeeNode sketch required for this module can be found in .../contrib/arduino/36_SHC_TEMP-pcaSerial.zip.<br><br>

  <a name="SHC_TEMPDefine"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; SHC_TEMP &lt;addr&gt; &lt;channel&gt;</code> <br>
    <br>
    addr is a 6 digit hex number to identify the SHC_TEMP device.
    channel is a 2 digit hex number to identify the SHC_TEMP device.<br><br>
    Note: devices are autocreated on reception of the first message.<br>
  </ul>
  <br>

  <a name="SHC_TEMP_Set"></a>
  <b>Set</b>
  <ul>
    <li>on</li>
    <li>off</li>
    <li>identify<br>
      Blink the status led for ~5 seconds.</li>
    <li>reset<br>
      Reset consumption counters</li>
    <li>statusRequest<br>
      Request device status update.</li>
    <li><a href="#setExtensions"> set extensions</a> are supported.</li>
  </ul><br>

  <a name="SHC_TEMP_Get"></a>
  <b>Get</b>
  <ul>
  </ul><br>

  <a name="SHC_TEMP_Readings"></a>
  <b>Readings</b>
  <ul>
    <li>power</li>
    <li>consumption</li>
    <li>consumptionTotal<br>
      will be created as a default user reading to have a continous consumption value that is not influenced
      by the regualar reset or overflow of the normal consumption reading</li>
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
