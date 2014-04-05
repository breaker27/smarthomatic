#!/bin/sh

SHC_HEX=`ls -x *.hex|head -n 1`

# detect ATMega type by file name
SHC_PROC=m328p
if [ ${SHC_HEX} = "shc_SOME168DEVICE.hex" ] ; then
  SHC_PROC=m168
fi

echo
echo "Firmware: ${SHC_HEX}"
echo "ATMega:   ${SHC_PROC}"
echo
echo "Do you want to flash firmware now? (Abort with Ctrl+C)"

read line
avrdude -p ${SHC_PROC} -U flash:w:${SHC_HEX}
