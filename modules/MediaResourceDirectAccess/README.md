# Media Resource Direct Access Library and sample

## Introduction

### Overview
Media Resource Direct Access feature enables virtual machines to access host hardware resources for hardware codec, even when a virtual GPU is not allocated to the virtual machine.
This functionality leverages the IVSHMEM shared memory technology to facilitate seamless data interaction between the virtual machines and the host system.
In addition, gRPC technology is utilized to achieve signal and data interaction between the virtual machine and the host system. A session manager is implemented on the host to control the lifecycle of multiple microservices, while a resource manager is responsible for querying the host's hardware resource utilization. Each host service manages an input and an output buffer queue.
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
- ffmpeg

### How to build MRDA Host Service
```
cd Scripts/Linux
./install_host.sh y # build with external library, input y; only build MRDA host service, input n
# codec option can be set in install_host.sh with -DVPL_SUPPORT -DFFMPEG_SUPPORT
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

### IVSHMEM device setup
In Linux Host side, each VM needs two shmem devices for in and out channels. There are two ways to setup IVSHMEM device.
- Hot-plogging mode:
```
cd ../shmem/host/scripts
sh ivshmem-device-setup.sh $vm_name 1G /dev/shm/shm1IN shm1IN 0x11
sh ivshmem-device-setup.sh $vm_name 1G /dev/shm/shm1OUT shm1OUT 0x12
```
- configure mode:
```
<shmem name='shm1IN'>
    <model type='ivshmem-plain'/>
    <size unit='M'>1024</size>
    <address type='pci' domain='0x0000' bus='0x00' slot='0x11' function='0x0'/>
</shmem>
<shmem name='shm1OUT'>
    <model type='ivshmem-plain'/>
    <size unit='M'>1024</size>
    <address type='pci' domain='0x0000' bus='0x00' slot='0x12' function='0x0'/>
</shmem>
```
In Windows Guest VM, please refer to "IVSHMEM driver setup" chapter in [How to build and run share memory samples in guest VM](../shmem/guestVMs/README.md) .

### How to build and run MRDA Guest Sample
There are 3 MRDA examples:
- MDSCMRDASample: it demonstrates how to utilize the MultiDisplayScreenCapture library to capture the screen of a Windows virtual machine (VM) and leverage the MediaResourceDirectAccess feature to access the host machine's hardware resources for hardware-accelerated video encoding. Please refer to [MDSCMRDASample](../Examples/MDSCMRDASample/README.md) for more details.
- SampleDecodeApp: it demonstrates how to utilize the MediaResourceDirectAccess feature to access the host machine's hardware resources for hardware-accelerated video decoding. Please refer to [SampleDecodeApp](../Examples/SampleDecodeApp/README.md) for more details.
- SampleEncodeApp: it demonstrates how to utilize the MediaResourceDirectAccess feature to access the host machine's hardware resources for hardware-accelerated video encoding. Please refer to [SampleEncodeApp](../Examples/SampleEncodeApp/README.md) for more details.