@echo off
@REM download grpc https://github.com/grpc/grpc/blob/v1.62.0/BUILDING.md
if not exist "..\..\..\external" (
    mkdir ..\..\..\external
)
if not exist "..\..\..\external\grpc" (
    cd ..\..\..\external
    git clone -b v1.62.0 https://github.com/grpc/grpc
    cd grpc
    git submodule update --init
) else (
    cd ..\..\..\external\grpc
)
@REM install grpc
if not exist "./install" (
    mkdir -p cmake\build
    cd cmake\build
    cmake ..\.. -G "Visual Studio 17 2022" -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="..\..\install" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --target install
)

cd "%~dp0"

if not exist "./build" (
    mkdir build
)
cd build
cmake .. -DCMAKE_PREFIX_PATH="%~dp0\..\..\..\external\grpc\install"
cmake --build . --config Release

cmake --install .