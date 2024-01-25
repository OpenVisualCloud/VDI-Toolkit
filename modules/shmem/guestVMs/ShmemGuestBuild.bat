@echo off

@rem Set workspace path and VS2022 path
set WorkspacePath=%~dp0
echo Workspacepath = %WorkspacePath%
echo.
echo "VS path is %VS2022INSTALLDIR%"
set PATH=%PATH%;%VS2022INSTALLDIR%\Msbuild\Current\bin;

@rem main func
goto:main

@rem Build All
:main
call:SetBuildParams
call:BuildConsole
call:BuildIvshmemApplication
goto:eof

@rem Build Console project
:BuildConsole
echo "Build Console..."
MSBuild.exe "%WorkspacePath%\Console\vioser_win.sln" -t:Rebuild %BuildTargetCfg% -v:normal -m
if %ERRORLEVEL% neq 0 echo "Build error!"
goto:eof

@rem Build Ivshmem-applications project
:BuildIvshmemApplication
echo "Build Ivshmem-applications..."
MSBuild.exe "%WorkspacePath%\Ivshmem-applications\ivshmem-applications.sln" -t:Rebuild %BuildTargetCfg% -v:normal -m
if %ERRORLEVEL% neq 0 echo "Build error!"
goto:eof

@rem set build parameters
:SetBuildParams
set BuildType=Release
set BuildPlatform=x64
set BuildTargetCfg=/p:Configuration=%BuildType%;Platform=%BuildPlatform%
goto:eof
