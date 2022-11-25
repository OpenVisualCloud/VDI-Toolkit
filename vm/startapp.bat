cd C:\Program Files (x86)\Windows Application Driver
timeout /T 60
for /f "tokens=2 delims=:(" %%a in ('ipconfig /all^|findstr /c:"IPv4"') do (
set "IP=%%a" goto StartWinAppDriver
)
 
:StartWinAppDriver
echo IP: %IP%
winappdriver.exe %IP% 4723
