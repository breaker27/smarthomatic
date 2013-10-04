@echo off

rem find files
for %%F IN (*.hex) DO set SHC_HEX=%%F

echo.
echo Firmware: %SHC_HEX%
echo.
echo Do you want to flash firmware now? (Abort with Ctrl+C)
echo.
pause
avrdude -p m328p -U flash:w:%SHC_HEX%
pause
