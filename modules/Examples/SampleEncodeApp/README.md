# SampleEncodeApp

SampleEncodeApp is a sample application that demonstrates how to use the `MediaResourceDirectAccess` module to encode a video stream using host hardware acceleration in guest VMs.

## Prerequisites

- Visual Studio 17 2022
- grpc == v1.62.0
- cmake >= 3.0 (prefer 3.29.3)
- git >= 2.0 (prefer 2.37.0)

## Building
- IVSHMEM device setup
Please refer to [IVSHMEM](../../MediaResourceDirectAccess/README.md) for IVSHMEM device setup.
- Build MRDA windows guest library
```
cd MediaResourceDirectAccess/Scripts/Windows/lib
./WinBuild.bat
```
- Build MRDA windows guest app
```
cd Examples/SampleEncodeApp/scripts
./WinBuild.bat
```
- Build MRDA host service
```
cd MediaResourceDirectAccess/Scripts/Linux
./install_host.sh y # build with external library, input y; only build MRDA host service, input n
# codec option can be set in install_host.sh with -DVPL_SUPPORT -DFFMPEG_SUPPORT
```

## Running
- Run MRDA host service
```
cd MediaResourceDirectAccess/Scripts/Linux/build
sudo ./HostService -addr 127.0.0.1:50051
```
- Run MRDA windows guest app
```
cd Examples/SampleEncodeApp/scripts/build/Release
 .\MRDASampleEncodeApp.exe --hostSessionAddr 127.0.0.1:50051 -i input.rgba -o output.hevc --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType ffmpeg
```