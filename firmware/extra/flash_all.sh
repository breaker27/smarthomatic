#!/bin/sh

SHC_E2P=`ls -x *.e2p|head -n 1`
SHC_HEX=`ls -x *.hex|head -n 1`

# detect ATMega type by file name
SHC_PROC=m328p
if [ ${SHC_HEX} = "shc_SOME168DEVICE.hex" ] ; then
  SHC_PROC=m168
fi

echo
echo "Firmware: ${SHC_HEX}"
echo "EEPROM:   ${SHC_E2P}"
echo "ATMega:   ${SHC_PROC}"
echo
echo "Do you want to flash fuses, eeprom and firmware now? (Abort with Ctrl+C)"

read line
avrdude -p ${SHC_PROC} -U flash:w:${SHC_HEX} -U eeprom:w:${SHC_E2P}:r -U lfuse:w:lfuse.bin:r -U hfuse:w:hfuse.bin:r -U efuse:w:efuse.bin:r
