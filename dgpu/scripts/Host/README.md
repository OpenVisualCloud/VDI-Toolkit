# Host Setup Guidance
## Introduction

The purpose of the file is to guide user to set GPU SRIOV with Intel Data Center GPU (Flex series) on CentOS Server Platform. The whole process will include server BIOS settings, kvm host setup and vGPU and VM setup.

## BIOS configuration

### MMIO

To support SR-IOV, it is crucial to ensure that there is enough MMIO address space reserved for future possible VF derived from PCIe devices that support SR-IOV during the BIOS power-on initialization.
Take Intel M50 CYP server BIOS configuration for example:
- Advanced -> PCI Configuration -> Memory Mapped I/O above 4GB : Enabled
- Advanced -> PCI Configuration -> MMIO High Base : 56T
- Advanced -> PCI Configuration -> Memory Mapped I/O Size : 1024G

### Enable virtualization

Virtualization Technology for Directed I/O, knowns as VT-d, is an Intel technology that enhances virtualization capabilities by providing hardware support for isolating and managing I/O devices in a virtualized environment. The location of VT-d configuration in the BIOS may be in Advanced->Processor Configuration or Advanced->System Agent(SA) Configuration, and its indicator in the BIOS is generally ”Intel(R) VT for Directed I/O” or “Intel VT-d”.
Take Intel M50 CYP server BIOS configuration for example:

- Advanced -> PCI Configuration -> SR-IOV Support : Enabled
- Advanced -> Processor Configuration -> Intel(R) Virtualization Technology: Enabled
- Advanced -> Integrated IO Configuration -> Intel(R) VT for Directed I/O : Enabled

### Fan setting

Since graphic card temperature rise will have a variety of hardware problems. So it is necessary to set FAN speed to maximum.
Take Intel M50 CYP server BIOS configuration for example:

- Advanced -> System Acoustic and Performance Configuration -> Set Fan Profile : Performance

## KVM host setup

### Download & install Flex series GPU driver

Download the ATS-M Linux build driver on host. The install script called install-sg2.sh shall be in the un-zipped folder. Please contact us if you require the access for Flex series GPU drivers.

### Install QEMU/KVM relative packages

QEMU is a fast and versatile open-source machine emulator and virtualizer. The following commands can be used to install QEMU and other relative tools:
```bash
yum install centos-release-qemu-ev
yum install -y qemu-img qemu-kvm-common qemu-kvm virt-manager libvirt libvirt-python virt-manager libvirt-client virt-install virt-viewer
```

### Tune kernel parameters

Tune grub parameters enabling GPU SRIOV support: edit /etc/default/grub and add following parameters in the “GRUB_CMDLINE_LINUX” session:
- i915.force_probe=*
- i915.enable_rc6=0
- i915.max_vfs=16
- drm.debug=0xe debug
- modprobe.blacklist=ast,snd_hda_intel
- intel_pstate=disable
- intel_idle.max_cstate=1
- pci=realloc=off
- iommu=pt
- intel_iommu=on
- split_lock_detect=off

After the config changes, use grub2-mkconfig to make the change take effect:
```bash
grub2-mkconfig -o /boot/efi/EFI/centos/grub.cfg
```
Reboot the OS to take effect.
```bash
reboot
```
After the OS boot up again, check virtualization status in the OS like:
```bash
virt-host-validate | grep QEMU
QEMU: Checking for hardware virtualization                          : PASS
QEMU: Checking if device /dev/kvm exists                            : PASS
QEMU: Checking if device /dev/kvm is accessible                     : PASS
QEMU: Checking if device /dev/vhost-net exists                      : PASS
QEMU: Checking if device /dev/net/tun exists                        : PASS
QEMU: Checking for cgroup 'cpu' controller support                  : PASS
QEMU: Checking for cgroup 'cpuacct' controller support              : PASS
QEMU: Checking for cgroup 'cpuset' controller support               : PASS
QEMU: Checking for cgroup 'memory' controller support               : PASS
QEMU: Checking for cgroup 'devices' controller support              : PASS
QEMU: Checking for cgroup 'blkio' controller support                : PASS
QEMU: Checking for device assignment IOMMU support                  : PASS
QEMU: Checking if IOMMU is enabled by kernel                        : PASS
grep -r . /sys/class/drm/card*/iov/mode
SR-IOV PF
```

### Install XPU manager

For installation, you can refer to [xpumanager](https://github.com/intel/xpumanager/tree/master).
Detaied guide for XPU manager, please refer to [xpumanager usage](https://github.com/intel/xpumanager/blob/master/README.md).

## vGPU and VM setup

Please refer to [VF & VM setup script guide](VM_scripts.md).