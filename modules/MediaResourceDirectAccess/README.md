# Media Resource Direct Access Library and sample

## Introduction

### Overview
Media Resource Direct Access feature enables virtual machines to access host hardware resources for hardware encoding, even when a virtual GPU is not allocated to the virtual machine.
This functionality leverages the IVSHMEM shared memory technology to facilitate seamless data interaction between the virtual machines and the host system.
In addition, gRPC technology is utilized to achieve signal and data interaction between the virtual machine and the host system. A session manager is implemented on the host to control the lifecycle of multiple encoding microservices, while a resource manager is responsible for querying the host's hardware resource utilization. Each encoding service manages an input and an output buffer queue.
The software diagram is shown in Figure 1.
<div style="text-align:center;">
 <img src="doc/MRDA%20SW%20diagram.png" alt="Media Resource Direct Access software diagram" width="800">

**Figure 1**: Media Resource Direct Access software diagram
</div>

## Host build

### Prerequisite
- git >= 2.0
- grpc == v1.62.0
- cmake >=3.0
- oneVPL

### How to build MRDA Host Service
```
cd Scripts/Linux
./install_host.sh y # build with external library, input y; only build MRDA host service, input n
```

## Guest build

### Prerequisite
- Visual Studio 17 2022
- grpc == v1.62.0
- cmake >= 3.0 (prefer 3.29.3)
- git >= 2.0 (prefer 2.37.0)

### How to build MRDA Guest Library
```
cd Scripts/Windows/lib
./WinBuild.bat
```

### How to build MRDA Guest Sample
```
cd Scripts/Windows/app
./WinBuild.bat
```

## Sample Run

### IVSHMEM device setup
In Linux Host side, each VM needs two shmem devices for in and out channels.
```
cd ../shmem/host/scripts
sh ivshmem-device-setup.sh $vm_name 1G /dev/shm/shm1IN shm1IN 0x11
sh ivshmem-device-setup.sh $vm_name 1G /dev/shm/shm1OUT shm1OUT 0x12
```

In Windows Guest VM, please refer to "IVSHMEM driver setup" chapter in [How to build and run share memory samples in guest VM](../shmem/guestVMs/README.md) .

### Run scripts
In Linux Host side:
```
cd Scripts/Linux/build
sudo ./HostService -addr 127.0.0.1:50051
```
In Windows Guest side:
```
cd Scripts/Windows/app/build/Release
 .\MRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 -i input.rgba -o output.hevc --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --gopRefDist 1 --numRefFrame 1
```
