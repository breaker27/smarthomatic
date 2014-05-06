# Copied from 36_JeeLink.pm and adapted
# $Id: 36_JeeLink.pm 3914 2013-09-16 13:35:50Z justme1968 $

package main;

use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

sub SHC_Attr(@);
sub SHC_Clear($);
sub SHC_HandleWriteQueue($);
sub SHC_Parse($$$$);
sub SHC_Read($);
sub SHC_ReadAnswer($$$$);
sub SHC_Ready($);
sub SHC_Write($$);

sub SHC_SimpleWrite(@);

my $clientsSHC = ":SHC_Dev:BASE:xxx:";

my %matchListSHC = (
  "1:SHC_Dev" => "^Packet Data: SenderID=[1-9]|0[1-9]|[1-9][0-9]|[0-9][0-9][0-9]|[0-3][0-9][0-9][0-9]|40[0-8][0-9]|409[0-6]",    #1-4096 with leading zeros
  "2:xxx"     => "^\\S+\\s+22",
  "3:xxx"     => "^\\S+\\s+11",
  "4:xxx"     => "^\\S+\\s+9 ",
);

sub SHC_Initialize($)
{
  my ($hash) = @_;

  require "$attr{global}{modpath}/FHEM/DevIo.pm";

  # Provider
  $hash->{ReadFn}  = "SHC_Read";
  $hash->{WriteFn} = "SHC_Write";
  $hash->{ReadyFn} = "SHC_Ready";

  # Normal devices
  $hash->{DefFn}      = "SHC_Define";
  $hash->{UndefFn}    = "SHC_Undef";
  $hash->{GetFn}      = "SHC_Get";
  $hash->{SetFn}      = "SHC_Set";
  $hash->{ShutdownFn} = "SHC_Shutdown";
}

#####################################
sub SHC_Define($$)
{
  my ($hash, $def) = @_;
  my @a = split("[ \t][ \t]*", $def);

  if (@a != 3) {
    my $msg = "wrong syntax: define <name> SHC {devicename[\@baudrate] " . "| devicename\@directio}";
    Log3 undef, 2, $msg;
    return $msg;
  }

  DevIo_CloseDev($hash);

  my $name = $a[0];
  my $dev  = $a[2];
  $dev .= "\@19200" if ($dev !~ m/\@/);

  $hash->{Clients}    = $clientsSHC;
  $hash->{MatchList}  = \%matchListSHC;
  $hash->{DeviceName} = $dev;

  my $ret = DevIo_OpenDev($hash, 0, "SHC_DoInit");
  return $ret;
}

#####################################
sub SHC_Undef($$)
{
  my ($hash, $arg) = @_;
  my $name = $hash->{NAME};

  foreach my $d (sort keys %defs) {
    if ( defined($defs{$d})
      && defined($defs{$d}{IODev})
      && $defs{$d}{IODev} == $hash)
    {
      my $lev = ($reread_active ? 4 : 2);
      Log3 $name, $lev, "$name: deleting port for $d";
      delete $defs{$d}{IODev};
    }
  }

  SHC_Shutdown($hash);
  DevIo_CloseDev($hash);
  return undef;
}

#####################################
sub SHC_Shutdown($)
{
  my ($hash) = @_;
  return undef;
}

#####################################
sub SHC_Set($@)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, @a) = @_;

  my $name = shift @a;
  my $cmd  = shift @a;
  my $arg  = join("", @a);

  my $list = "raw:noArg";
  return $list if ($cmd eq '?');

  if ($cmd eq "raw") {

    #return "\"set SHC $cmd\" needs exactly one parameter" if(@_ != 4);
    #return "Expecting a even length hex number" if((length($arg)&1) == 1 || $arg !~ m/^[\dA-F]{12,}$/ );
    Log3 $name, 4, "$name: set $name $cmd $arg";
    SHC_SimpleWrite($hash, $arg);

  } else {
    return "Unknown argument $cmd, choose one of " . $list;
  }

  return undef;
}

#####################################
sub SHC_Get($@)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $name, $cmd) = @_;

  my $list = "devices:noArg initSHC:noArg";

  if ($cmd eq "devices") {
    SHC_SimpleWrite($hash, "l");
  } elsif ($cmd eq "initSHC") {
    SHC_SimpleWrite($hash, "0c");
    SHC_SimpleWrite($hash, "2c");
  } else {
    return "Unknown argument $cmd, choose one of " . $list;
  }

  return undef;
}

sub SHC_Clear($)
{
  my $hash = shift;

  # Clear the pipe
  $hash->{RA_Timeout} = 0.1;
  for (; ;) {
    my ($err, undef) = SHC_ReadAnswer($hash, "Clear", 0, undef);
    last if ($err && $err =~ m/^Timeout/);
  }
  delete($hash->{RA_Timeout});
}

#####################################
sub SHC_DoInit($)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my $hash = shift;
  my $name = $hash->{NAME};
  my $err;
  my $msg = undef;

  my $val;

  $hash->{STATE} = "Initialized";

  # Reset the counter
  delete($hash->{XMIT_TIME});
  delete($hash->{NR_CMD_LAST_H});
  return undef;
}

#####################################
# This is a direct read for commands like get
# Anydata is used by read file to get the filesize
sub SHC_ReadAnswer($$$$)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $arg, $anydata, $regexp) = @_;
  my $type = $hash->{TYPE};
  my $name = $hash->{NAME};

  return ("No FD", undef)
    if (!$hash || ($^O !~ /Win/ && !defined($hash->{FD})));

  my ($mpandata, $rin) = ("", '');
  my $buf;
  my $to = 3;    # 3 seconds timeout
  $to = $hash->{RA_Timeout} if ($hash->{RA_Timeout});    # ...or less
  for (; ;) {

    if ($^O =~ m/Win/ && $hash->{USBDev}) {
      $hash->{USBDev}->read_const_time($to * 1000);      # set timeout (ms)
                                                         # Read anstatt input sonst funzt read_const_time nicht.
      $buf = $hash->{USBDev}->read(999);
      return ("Timeout reading answer for get $arg", undef)
        if (length($buf) == 0);

    } else {
      return ("Device lost when reading answer for get $arg", undef)
        if (!$hash->{FD});

      vec($rin, $hash->{FD}, 1) = 1;
      my $nfound = select($rin, undef, undef, $to);
      if ($nfound < 0) {
        next if ($! == EAGAIN() || $! == EINTR() || $! == 0);
        my $err = $!;
        DevIo_Disconnected($hash);
        return ("SHC_ReadAnswer $arg: $err", undef);
      }
      return ("Timeout reading answer for get $arg", undef)
        if ($nfound == 0);
      $buf = DevIo_SimpleRead($hash);
      return ("No data", undef) if (!defined($buf));

    }

    if ($buf) {
      Log3 $hash->{NAME}, 5, "$name: SHC/RAW (ReadAnswer): $buf";
      $mpandata .= $buf;
    }

    chop($mpandata);
    chop($mpandata);

    return (
      undef, $mpandata
      );
  }

}

#####################################
# Check if the 1% limit is reached and trigger notifies
sub SHC_XmitLimitCheck($$)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $fn) = @_;
  my $now = time();

  if (!$hash->{XMIT_TIME}) {
    $hash->{XMIT_TIME}[0] = $now;
    $hash->{NR_CMD_LAST_H} = 1;
    return;
  }

  my $nowM1h = $now - 3600;
  my @b = grep {$_ > $nowM1h} @{$hash->{XMIT_TIME}};

  if (@b > 163) {    # 163 comes from fs20. todo: verify if correct for SHC modulation

    my $name = $hash->{NAME};
    Log3 $name, 2, "$name: SHC TRANSMIT LIMIT EXCEEDED";
    DoTrigger($name, "TRANSMIT LIMIT EXCEEDED");

  } else {

    push(@b, $now);

  }
  $hash->{XMIT_TIME}     = \@b;
  $hash->{NR_CMD_LAST_H} = int(@b);
}

#####################################
sub SHC_Write($$)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $msg) = @_;
  my $name = $hash->{NAME};

  Log3 $name, 5, "$name: sending $msg";

  SHC_AddQueue($hash, $msg);

  #SHC_SimpleWrite($hash, $msg);
}

sub SHC_SendFromQueue($$)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $bstring) = @_;
  my $name = $hash->{NAME};
  my $to   = 0.05;

  if ($bstring ne "") {
    my $sp = AttrVal($name, "sendpool", undef);
    if ($sp) {    # Is one of the SHC-fellows sending data?
      my @fellows = split(",", $sp);
      foreach my $f (@fellows) {
        if ( $f ne $name
          && $defs{$f}
          && $defs{$f}{QUEUE}
          && $defs{$f}{QUEUE}->[0] ne "")
        {
          unshift(@{$hash->{QUEUE}}, "");
          InternalTimer(gettimeofday() + $to, "SHC_HandleWriteQueue", $hash, 1);
          return;
        }
      }
    }

    SHC_XmitLimitCheck($hash, $bstring);
    SHC_SimpleWrite($hash, $bstring);
  }

  InternalTimer(gettimeofday() + $to, "SHC_HandleWriteQueue", $hash, 1);
}

sub SHC_AddQueue($$)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash, $bstring) = @_;
  if (!$hash->{QUEUE}) {
    $hash->{QUEUE} = [$bstring];
    SHC_SendFromQueue($hash, $bstring);

  } else {
    push(@{$hash->{QUEUE}}, $bstring);
  }
}

#####################################
sub SHC_HandleWriteQueue($)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my $hash = shift;
  my $arr  = $hash->{QUEUE};
  if (defined($arr) && @{$arr} > 0) {
    shift(@{$arr});
    if (@{$arr} == 0) {
      delete($hash->{QUEUE});
      return;
    }
    my $bstring = $arr->[0];
    if ($bstring eq "") {
      SHC_HandleWriteQueue($hash);
    } else {
      SHC_SendFromQueue($hash, $bstring);
    }
  }
}

#####################################
# called from the global loop, when the select for hash->{FD} reports data
sub SHC_Read($)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash) = @_;

  my $buf = DevIo_SimpleRead($hash);
  return "" if (!defined($buf));

  my $name = $hash->{NAME};

  my $pandata = $hash->{PARTIAL};
  Log3 $name, 5, "$name: SHC/RAW: $pandata/$buf";
  $pandata .= $buf;

  while ($pandata =~ m/\n/) {
    my $rmsg;
    ($rmsg, $pandata) = split("\n", $pandata, 2);
    $rmsg =~ s/\r//;
    SHC_Parse($hash, $hash, $name, $rmsg) if ($rmsg);
  }
  $hash->{PARTIAL} = $pandata;
}

sub SHC_Parse($$$$)
{
  my ($hash, $iohash, $name, $rmsg) = @_;
  my $dmsg = $rmsg;

  next if (!$dmsg || length($dmsg) < 1);    # Bogus messages

  if ($dmsg !~ m/^Packet Data: SenderID=/) {

    # Messages just to dipose
    if ( $dmsg =~ m/^\*\*\* Enter AES key nr/
      || $dmsg =~ m/^\*\*\* Received character/)
    {
      return;
    }

    # -Verbosity level 5
    if ( $dmsg =~ m/^Received \(AES key/
      || $dmsg =~ m/^Received garbage/
      || $dmsg =~ m/^Before encryption/
      || $dmsg =~ m/^After encryption/
      || $dmsg =~ m/^Repeating request./
      || $dmsg =~ m/^Request Queue empty/
      || $dmsg =~ m/^Removing request from request buffer/)
    {
      Log3 $name, 5, "$name: $dmsg";
      return;
    }

    # -Verbosity level 4
    if ( $dmsg =~ m/^Request added to queue/
      || $dmsg =~ m/^Request Buffer/
      || $dmsg =~ m/^Request (q|Q)ueue/)
    {
      Log3 $name, 4, "$name: $dmsg";
      return;
    }

    # Anything else in verbosity level 3
    Log3 $name, 3, "$name: $dmsg";
    return;
  }

  $hash->{"${name}_MSGCNT"}++;
  $hash->{"${name}_TIME"} = TimeNow();
  $hash->{RAWMSG} = $rmsg;
  my %addvals = (RAWMSG => $rmsg);

  Dispatch($hash, $dmsg, \%addvals);
}

#####################################
sub SHC_Ready($)
{
  # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
  my ($hash) = @_;

  return DevIo_OpenDev($hash, 1, "SHC_DoInit")
    if ($hash->{STATE} eq "disconnected");

  # This is relevant for windows/USB only
  my $po = $hash->{USBDev};
  my ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags);
  if ($po) {
    ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags) = $po->status;
  }
  return ($InBytes && $InBytes > 0);
}

########################
sub SHC_SimpleWrite(@)
{
  my ($hash, $msg) = @_;
  return if (!$hash);

  my $name = $hash->{NAME};
  Log3 $name, 5, "$name: SW: $msg";

  $msg .= "\r";

  $hash->{USBDev}->write($msg) if ($hash->{USBDev});
  syswrite($hash->{DIODev}, $msg) if ($hash->{DIODev});

  # Some linux installations are broken with 0.001, T01 returns no answer
  select(undef, undef, undef, 0.01);
}

sub SHC_Attr(@)
{
  my @a = @_;

  return undef;
}

1;

=pod
=begin html

<a name="SHC"></a>
<h3>SHC</h3>
<ul>
  The SHC is a family of RF devices avaible at  <a href="http://http://www.smarthomatic.org">www.smarthomatic.org</a>.

  This module provides the IODevice for the <a href="#SHC_Dev">SHC_Dev</a> modules that implements the SHC_Dev protocol.<br><br>

  Note: this module may require the Device::SerialPort or Win32::SerialPort
  module if you attach the device via USB and the OS sets strange default
  parameters for serial devices.

  <br><br>

  <a name="SHC_Define"></a>
  <b>Define</b>
  <ul>
    <code>define &lt;name&gt; SHC &lt;device&gt;</code> <br>
    <br>
    USB-connected devices:<br><ul>
      &lt;device&gt; specifies the serial port to communicate with the SHC.
      The name of the serial-device depends on your distribution, under
      linux the cdc_acm kernel module is responsible, and usually a
      /dev/ttyACM0 device will be created. If your distribution does not have a
      cdc_acm module, you can force usbserial to handle the SHC by the
      following command:<ul>modprobe usbserial vendor=0x0403
      product=0x6001</ul>In this case the device is most probably
      /dev/ttyUSB0.<br><br>

      You can also specify a baudrate if the device name contains the @
      character, e.g.: /dev/ttyACM0@57600<br><br>

      If the baudrate is "directio" (e.g.: /dev/ttyACM0@directio), then the
      perl module Device::SerialPort is not needed, and fhem opens the device
      with simple file io. This might work if the operating system uses sane
      defaults for the serial parameters, e.g. some Linux distributions and
      OSX.  <br><br>

    </ul>
    <br>
  </ul>
  <br>

  <a name="SHC_Set"></a>
  <b>Set</b>
  <ul>
    <li>raw &lt;datar&gt;<br>
        # TODO: Not adapted to SHC, copy from 36_JeeLink.pm
        send &lt;data&gt; as a raw message to the SHC to be transmitted over the RF link.
        </li><br>
  </ul>

  <a name="SHC_Get"></a>
  <b>Get</b>
  <ul>
  </ul>

  <a name="SHC_Attr"></a>
  <b>Attributes</b>
  <ul>
  </ul>
  <br>
</ul>

=end html
=cut
