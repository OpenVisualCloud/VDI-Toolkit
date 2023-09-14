@echo off

setlocal EnableDelayedExpansion

REM Setup workspace path
set WorkspacePath=%~dp0
echo Workspace Path = %WorkspacePath%
echo.
echo VS path is %VS2022INSTALLDIR%
set PATH=%PATH%;%VS2022INSTALLDIR%\Msbuild\Current\bin;

echo Fetching deps...
echo .
set jsonURL=https://github.com/nlohmann/json/releases/download/v3.8.0/include.zip
set jsonOutput=%WorkspacePath%\include.zip
set jsonPath=%WorkspacePath%\tmp\nlohmann
mkdir %WorkspacePath%\MediaSample\deps\include\nlohmann
set jsonIncludePath=%WorkspacePath%\MediaSample\deps\include\nlohmann
powershell.exe -c "Invoke-WebRequest -Uri %jsonURL% -OutFile %jsonOutput%"
powershell.exe -c "Expand-Archive %jsonOutput% -DestinationPath %jsonPath%"
xcopy /s /y %jsonPath%\include\nlohmann %jsonIncludePath%


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

del /q %jsonOutput%
rd /s /q tmp

echo.
echo Start building Testapp ...
title Building Testapp ...

MSBuild.exe "MediaSample.sln" -t:Rebuild %BuildTargetCfg% -v:normal -m
echo.
echo ... Building Testapp = %ERRORLEVEL%
if %ERRORLEVEL% neq 0 goto :build_failed

mkdir bin
xcopy /y MediaSample\deps\bin\QESLib.dll bin\
MOVE /y x64\Release\*.exe bin\
xcopy /y MediaSample\config\*.json bin\
rd /s /q x64
rd /s /q MediaSample\x64
