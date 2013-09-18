#!/usr/bin/perl -w

# Set up the serial port
use Device::SerialPort;
my $port = Device::SerialPort->new("/dev/ttyUSB0");

# 19200, 81N on the USB ftdi driver
$port->baudrate(19200); # you may change this value
$port->databits(8); # but not this and the two following
$port->parity("none");
$port->stopbits(1);

$port->write("h");

# Wait one second
sleep(1);

my $line = "";

while (1)
{
  my $chr;
  
  while(!defined($chr) || !length($chr) )
  {
    $chr = $port->read(1);
  }
    
  $line .= $chr;

  if ($chr eq "\n")
  {
    $line =~ s/[\r\n]*//g;
  
    if ($line ne "")
    {
      $time = `date "+%Y-%m-%d %H:%M:%S"`;
      $time =~ s/[\r\n]*//g;
      print $time . ": " . $line . "\n";
    }
    
    $line = "";
  }
}
