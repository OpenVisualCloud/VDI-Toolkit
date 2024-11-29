@echo off
@REM build sample app
cd "%~dp0"

if not exist "./build" (
    mkdir build
)
cd build
cmake .. -DENABLE_TRACE=OFF
cmake --build . --config Release
xcopy "..\..\..\MediaResourceDirectAccess\Scripts\Windows\lib\install\lib\libWinGuest.dll" "Release\" /I /Y
for /r "..\..\..\multidispscreencap\bin" %%a in (*.dll) do xcopy "%%a" "Release\" /I /Y
xcopy "..\..\..\multidispscreencap\bin\mediamtx_v1.6.0_windows_amd64" "Release\mediamtx_v1.6.0_windows_amd64\" /E /S /I
