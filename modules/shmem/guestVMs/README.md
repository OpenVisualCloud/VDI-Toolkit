# How to build and run share memory samples in guest VM

## VM driver configuration

- IVSHMEM driver setup

The Inter-VM shared memory device (ivshmem) is designed to share a memory region between multiple QEMU processes running different guests and the host.  In order for all guests to be able to pick up the shared memory area, it is modeled by QEMU as a PCI device exposing said memory to the guest as a PCI BAR.

Inter-VM shared memory communication is a hardware-neutral feature.
Guest OS is required to have an IVSHMEM driver, such as virtio-WIN for Windows and ivshmem APIs.

To install the IVSHMEM driver, please follow these steps:
1. Open the device manager on your computer.
2. Locate the device named "PCI standard RAM Controller" under the "System Devices" section.
3. Right-click on the device and select "Update driver".
4. Browse my computer for driver software.

you can obtain the signed windows driver from Red Hat by visiting [IVSHMEM Driver](https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/upstream-virtio/).

- Virtio serial driver setup

Virtio serial modifies the current single-port virtio-console device to guests running on top of qemu and kvm. It exposes multiple ports to the guest in the form of simple char devices for simple IO between the guest and host userspaces. It also allows for multiple such devices to be exposed, lifting the current single device restriction.

To install the virtio serial driver, you need to download virtio-win image from [here](https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.215-2/virtio-win-0.1.215.iso) and run virtio-win-guest-tools.exe in the image to complete the installation.

## Guest VM build

- ivshmem-applications

For v23.12 release, we only have one share-memory case: screen capture.

Open ivshmem-applications.sln and build the solution in default config. An binary file IVScreenCapture.exe will appear in \x64\Release folder.
The usage for the IVScreenCapture is:
```
Usage: IVScreenCapture.exe [<options>]
Options:
    [--help, -h]             - print help README document.
    [--fps, -f number]       - specifies screen capture fps.
    [--framesNum, -n number] - specifies a total number of screen frames to capture.
Examples: IVScreenCapture.exe --fps 30 -n 300
```

- Console

For guest main application, Console provides the connection between host and guest VM.
```
cd \guestVMs\Console
# Open vioser_win.sln and build the solution in default config.
cd \x64\Release
.\vioser_win.exe
```

You can set the shortcut file containing "vioser_win.exe" to be launched at startup, so that the guest VM will always be in a state of listening to the host.
