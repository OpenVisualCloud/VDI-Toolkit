@echo off

setlocal EnableDelayedExpansion

REM Setup MultiDisplayScreenCapture solution path
set SolutionPath=%~dp0
echo MultiDisplayScreenCapture Solution Path = %SolutionPath%
REM Setup MultiDisplayScreenCapture project path
set MDSCLibProjectPath=%SolutionPath%MultiDisplayScreenCapture
echo MultiDisplayScreenCapture Project Path = %MDSCLibProjectPath%
REM Setup MDSCSample project path
set MDSCSampleProjectPath=%SolutionPath%MDSCSample
echo MDSCSample Project Path = %MDSCSampleProjectPath%

echo.
echo VS path is %VS2022INSTALLDIR%
set PATH=%PATH%;%VS2022INSTALLDIR%\Msbuild\Current\bin

echo.
echo WinRAR path is %WinRARPath%
set PATH=%PATH%;%WinRARPath%

echo Fetching deps...
echo.
set depsPath=%MDSCSampleProjectPath%\deps
echo deps path is %depspath%
mkdir %depspath%
echo Fetching nlohmann/json.hpp ...
set jsonurl=https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
set jsonpath=%depspath%\nlohmann
set jsonoutput=%jsonpath%\json.hpp
mkdir %jsonpath%
powershell.exe -c "invoke-webrequest -uri %jsonurl% -outfile %jsonoutput%"
set jsonincludepath=%jsonpath%
echo json include path is %jsonincludepath%

echo.
echo Fetching ffmpeg-6.1.1-full_build-shared ...
set ffmpegurl=https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z
set ffmpegpath=%depsPath%\ffmpeg-6.1.1-full_build-shared
set ffmpegoutput=%depsPath%\ffmpeg-6.1.1-full_build-shared.7z
powershell.exe -c "Invoke-WebRequest -Uri %ffmpegurl% -OutFile %ffmpegoutput%"
WinRAR.exe x %ffmpegoutput% *.* %depsPath%
set ffmpegincludepath=%ffmpegpath%\include
set ffmpegdllpath=%ffmpegpath%\bin
set ffmpeglibpath=%ffmpegpath%\lib
echo ffmpeg include path is %ffmpegincludepath%

REM Select build type (Release/Debug), platform (x64/Win32)
REM Default set to Release|x64
:parse_args
set BuildType=Release
set BuildPlatform=x64
set BuildPlatformToolset=v143
set BuildVersion=

if "%__BUILD_HELP%" neq "" goto :usage

REM Selected build type and platfrom
echo Build Type     = %BuildType%
echo Build Platform = %BuildPlatform%
REM Setup build target configuration
set BuildTargetCfg=/p:Configuration=%BuildType%;Platform=%BuildPlatform%
echo Build Target   = %BuildTargetCfg%
echo Build Version  = %BuildVersion%

echo.
echo Start building MultiDisplayScreenCapture ...
title Building MultiDisplayScreenCapture ...

MSBuild.exe "MultiDisplayScreenCapture.sln" -t:Rebuild %BuildTargetCfg% -v:normal -m
echo.
echo ... Building MultiDisplayScreenCapture = %ERRORLEVEL%
if %ERRORLEVEL% neq 0 goto :build_failed

mkdir bin
echo ffmpeg dll path is %ffmpegdllpath%
xcopy /y %ffmpegdllpath%\*.dll bin\
xcopy /y %ffmpegdllpath%\ffplay.exe bin\
MOVE /y x64\Release\*.exe bin\
MOVE /y x64\Release\*.dll bin\
MOVE /y x64\Release\*.lib bin\
xcopy /y %MDSCSampleProjectPath%\*.conf bin\
xcopy /y %MDSCSampleProjectPath%\*.bat bin\
echo Fetching mediamtx_v1.6.0_windows_amd64 ...
set mediaMTXurl=https://github.com/bluenviron/mediamtx/releases/download/v1.6.0/mediamtx_v1.6.0_windows_amd64.zip
set mediaMTXoutput=bin\mediamtx_v1.6.0_windows_amd64.zip
set mediaMTXpath=bin\mediamtx_v1.6.0_windows_amd64
powershell.exe -c "Invoke-WebRequest -Uri %mediaMTXurl% -OutFile %mediaMTXoutput%"
mkdir %mediaMTXpath%
WinRAR.exe x %mediaMTXoutput% *.* %mediaMTXpath%
rd /s /q x64
rd /s /q %MDSCLibProjectPath%\x64
rd /s /q %MDSCSampleProjectPath%\x64

echo.
echo MDSC buils successfully.
pause