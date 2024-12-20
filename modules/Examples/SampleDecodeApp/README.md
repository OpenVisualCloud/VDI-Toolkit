# SampleDecodeApp

SampleDecodeApp is a sample application that demonstrates how to use the `MediaResourceDirectAccess` module to decode a video stream using host hardware acceleration in guest VMs.

## Prerequisites

- Visual Studio 17 2022
- grpc == v1.62.0
- cmake >= 3.0 (prefer 3.29.3)
- git >= 2.0 (prefer 2.37.0)
- ffmpeg-7.1

## Building
- IVSHMEM device setup
Please refer to [IVSHMEM](../../MediaResourceDirectAccess/README.md) for IVSHMEM device setup.
- Build MRDA windows guest library
```
cd MediaResourceDirectAccess/Scripts/Windows/lib
./WinBuild.bat
```
- Install windows ffmpeg library
Refer to [How to install windows ffmpeg library](Scripts/Windows/SampleDecode/prebuild.md)
- Build MRDA windows guest app
```
cd Examples/SampleDecodeApp/scripts
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
cd Examples/SampleDecodeApp/scripts/build/Release
./MRDASampleDecodeApp.exe --hostSessionAddr 127.0.0.1:50051 -i input.h265 -o output.raw --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --decodeType ffmpeg
```

# SampleDecodeAppSW

SampleDecodeAppSW is a sample application that demonstrates how to use ffmpeg software decoder to decode a video stream in guest VMs.

- Build and run ffmpeg software decoder app
```
cd Examples/SampleDecodeApp/scripts
./WinBuild.bat
```
```
cd Examples/SampleDecodeApp/scripts/build/Release
./MRDASampleDecodeAppSW.exe -i input.h265 -o output.yuv420p --frameNum 3000 --codecId h265 --fps 30 --width 1920 --height 1080 --colorFormat yuv420p --decodeType ffmpeg
```