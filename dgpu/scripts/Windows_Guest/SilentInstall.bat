@echo off
set driverPath=%~dp0\..\..\bin\Windows_Guest\Installer-Release-64-bit-win-PR4
set scriptPath=%~dp0
echo Driver Path is: %driverPath%
echo Script Path is: %scriptPath%
echo "Install Driver..."
@echo off 
start /wait %driverPath%\Installer.exe -s -f
echo "Creating Display..."
@echo off 
start /wait PowerShell.exe -ExecutionPolicy Bypass -command ".\%scriptPath%\install-Flex170.ps1 -enable \"2k\"" 
echo "Restarting OS in 10 sec..."
shutdown -r -t 10