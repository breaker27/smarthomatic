@echo off

rem find files
for %%F IN (*.hex) DO set SHC_HEX=%%F

rem detect ATMega type by device name
set SHC_PROC=m168
if %SHC_HEX%==shc_basestation.hex set SHC_PROC=m328p
if %SHC_HEX%==shc_dimmer.hex set SHC_PROC=m328p

echo.
echo Firmware: %SHC_HEX%
echo ATMega:   %SHC_PROC%
echo.
echo Do you want to flash firmware now? (Abort with Ctrl+C)
echo.
pause
avrdude -p %SHC_PROC% -U flash:w:%SHC_HEX%
pause
