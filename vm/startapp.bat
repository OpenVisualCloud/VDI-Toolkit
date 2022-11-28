regedit /s disable-error-report.reg
timeout /T 60
for /f "tokens=2 delims=:(" %%a in ('ipconfig /all^|findstr /c:"IPv4"') do (
set "IP=%%a" goto StartWinAppDriver
)
 
:StartWinAppDriver
echo IP: %IP%
start /WAIT winappdriver.exe %IP% 4723
echo "%ERRORLEVEL%"
IF %ERRORLEVEL% NEQ 0 goto StartWinAppDriver
timeout /T 2
