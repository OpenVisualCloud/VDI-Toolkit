@echo off
@REM build sample app

cd "%~dp0"

if not exist "./build" (
    mkdir build
)
cd build
cmake ..
cmake --build . --config Release