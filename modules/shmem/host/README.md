# How to build and run share memory samples in host

## VMs device configuration

- IVSHMEM device setup

The Inter-VM shared memory device (ivshmem) is designed to share a memory region between multiple QEMU processes running different guests and the host.  In order for all guests to be able to pick up the shared memory area, it is modeled by QEMU as a PCI device exposing said memory to the guest as a PCI BAR.

ivshmem-device-setup.sh is used for a single VM.
```
ivshmem-device-setup.sh $vm_name $mem_size $mem_path $mem_id $pci_addr
```
run_ivshmem.sh provides a sample for multi-VM ivshmem device setup. For example:
```
sh ivshmem-device-setup.sh win2k19-1 1G /dev/shm/shm1 shm1 0x11
sh ivshmem-device-setup.sh win2k19-2 1G /dev/shm/shm2 shm2 0x12
sh ivshmem-device-setup.sh win2k19-3 1G /dev/shm/shm3 shm3 0x13
sh ivshmem-device-setup.sh win2k19-4 1G /dev/shm/shm4 shm4 0x14
```

- Virtio serial device setup

Virtio serial modifies the current single-port virtio-console device to guests running on top of qemu and kvm. It exposes multiple ports to the guest in the form of simple char devices for simple IO between the guest and host userspaces. It also allows for multiple such devices to be exposed, lifting the current single device restriction.

vioserial-device-setup.sh is used for a single VM.
```
vioserial-device-setup.sh $vm_name $device_id $serial_path $serial_id $port_name
```
run_vioserial.sh provides a sample for multi-VM virtio serial device setup. For example:
```
sh vioserial-device-setup.sh win2k19-1 virtio-serial1 /tmp/serial1 serial1 port1
sh vioserial-device-setup.sh win2k19-2 virtio-serial2 /tmp/serial2 serial2 port2
sh vioserial-device-setup.sh win2k19-3 virtio-serial3 /tmp/serial3 serial3 port3
sh vioserial-device-setup.sh win2k19-4 virtio-serial4 /tmp/serial4 serial4 port4
```

## Host build

- oneVPL

For screen capture usage, oneVPL is utilized to encode screen raw data. To build oneVPL, please refer to [oneVPL](https://github.com/intel/libvpl/).
```
cd host
git clone https://github.com/intel/libvpl.git oneVPL
cd oneVPL
git checkout v2023.3.1
git am --ignore-whitespace ../oneVPL-patches/0001-Add-share-memory-file-process-in-sample-encode.patch
sudo script/bootstrap
script/build # recommend gcc verion: 7
```

- console

For host main application, console provides the connection between host and multiple guest VMs.
```
cd host/console
mkdir build && cd build
cmake .. && make
```
The following shows the usage of console:
```
sudo ./Connect
Usage: ./Connect [<options>]
Options:
    [--help, -h]                        - print help README document.
    [--vmconfig, -vmc config_file_path] - specifies vm connection configuration.
    [--enconfig, -enc config_file_path] - specifies encoding configuration.
    [--type application_type]           - specifies an application type. e.g., SC(ScreenCapture).
    [--framesNum, -n number]            - specifies a total number of screen frames to capture if type is SC.
    [--fps, -f number]                  - specifies a fps of screen capture if type is SC.
Examples: ./Connect --vmconfig ./vmconfig.txt --enconfig ./enconfig.txt --type SC -n 300 --fps 30
```

To connect multiple guest VMs to get their screen contents. The sample configuration is as below:

vmconfig.txt (xxx.xxx.xxx.xxx is vm ip.)
```
xxx.xxx.xxx.xxx /tmp/serial1
xxx.xxx.xxx.xxx /tmp/serial2
xxx.xxx.xxx.xxx /tmp/serial3
xxx.xxx.xxx.xxx /tmp/serial4
```

enconfig.txt (-shmfile: indicate share memory use case. -shmperf: fps performance test)
```
../../oneVPL/_build/sample_encode h264 -hw -i /dev/shm/shm1 -rgb4 -w 1920 -h 1080 -b 20000 -fps 30 -shmfile -shmperf -n 300 -o ./dump1.h264
../../oneVPL/_build/sample_encode h264 -hw -i /dev/shm/shm2 -rgb4 -w 1920 -h 1080 -b 20000 -fps 30 -shmfile -shmperf -n 300 -o ./dump2.h264
../../oneVPL/_build/sample_encode h264 -hw -i /dev/shm/shm3 -rgb4 -w 1920 -h 1080 -b 20000 -fps 30 -shmfile -shmperf -n 300 -o ./dump3.h264
../../oneVPL/_build/sample_encode h264 -hw -i /dev/shm/shm4 -rgb4 -w 1920 -h 1080 -b 20000 -fps 30 -shmfile -shmperf -n 300 -o ./dump4.h264
```

The host case is as the following command:
```
sudo ./Connect --vmconfig ./vmconfig.txt --enconfig ./enconfig.txt --type SC -n 300 --fps 30
```