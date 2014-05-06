@echo off

rem find files
for %%F IN (*.e2p) DO set SHC_E2P=%%F
for %%F IN (*.hex) DO set SHC_HEX=%%F

rem detect ATMega type by file name
set SHC_PROC=m328p
if %SHC_HEX%==shc_SOME168DEVICE.hex set SHC_PROC=m168

echo.
echo Firmware: %SHC_HEX%
echo EEPROM:   %SHC_E2P%
echo ATMega:   %SHC_PROC%
echo.
echo Do you want to flash fuses, eeprom and firmware now? (Abort with Ctrl+C)
echo.
pause
avrdude -p %SHC_PROC% -U flash:w:%SHC_HEX% -U eeprom:w:%SHC_E2P%:r -U lfuse:w:lfuse.bin:r -U hfuse:w:hfuse.bin:r -U efuse:w:efuse.bin:r
pause
