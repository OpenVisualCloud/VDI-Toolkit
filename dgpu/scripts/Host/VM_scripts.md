# VF & VM setup script guide

## Clone VMs instance
This script is used to clone an assigned number of qcow2 VM disk image from a source disk image. We assume that we have a well built qcow2 image already.
This script could create the assigned number of VMs instances.
Usage of this script shall be:
```bash
./0_clone_vm_instance.sh $qcow_dir $count
```
where qcow_dir is the directory of qcow images and count will be the count of the VMs.

##  Generate VFs
This script is used to xpumcli tool generate an assigned number of VFs from the assigned GPU.
The usage of this script shall be:
```bash
./1_vf_setup.sh $card_id $vf_count $lmem(GB)
```
where card_id is the GPU card id which can be obtained by /dev/dri/card$card_id, vf_count is the number of VFs and lmem is the memory for each VF with the unit of GB.
```bash
e.g.:(card 0)
./1_vf_setup_by_xpumcli.sh 0 16 1
./1_vf_setup_by_xpumcli.sh 0 8 2
./1_vf_setup_by_xpumcli.sh 0 4 4
./1_vf_setup_by_xpumcli.sh 0 2 8
./1_vf_setup_by_xpumcli.sh 0 1 16

e.g.:(card 1)
./1_vf_setup_by_xpumcli.sh 1 16 1
./1_vf_setup_by_xpumcli.sh 1 8 2
./1_vf_setup_by_xpumcli.sh 1 4 4
./1_vf_setup_by_xpumcli.sh 1 2 8
./1_vf_setup_by_xpumcli.sh 1 1 16
```
Notes:
The 1_vf_setup_by_xpumcli.sh script need install xpumcli tool.

## Assign vGPU to VMs
This script is used to assign VF to each VM. Normally we could assign 1 VF for each VM by simply running it.
The usage of this script shall be:
```bash
./2_assign_vGPU_to_VMs.sh
```

## Start VMs
This script is used to start all VMs that are generated with previous scripts. No specific params acquired.
The usage of this script shall be:
```bash
./3_start_VMs.sh
```

## Forward VMs ports
This script is used to map ports for all VM. VNC ports of VM will be set to 10001~10016 and RDP ports will be set to 20001~20016 according to VM IDs. No specific params acquired.
The usage of this script shall be:
```bash
./4_vm_port_forward.sh
```

## Shutdown VMs
This script is used to shutdown all VMs that are generated with previous scripts.
The usage of this script shall be:
```bash
./5_shutdown_VMs.sh
```

## Unplug VM vGPU
This script will unplug vGPU from a selected VM and copy the unplugged vGPU HW data into a selected xml.
The usage of this script shall be:
```bash
./6_VM-vgpu-unplug.sh $VM_NAME $target_XML
```
where VM_NAME is the name of VM and target_XML is an output xml file which contains the vGPU HW data config.
```bash
e.g. ./6_VM-vgpu-unplug.sh win2k19 device.xml
```
Notes:
this command will detach vGPU from VM named win2k19 and move the vGPU HW data config into a new file device.xml.
if we need to retach the vGPU, we could use virsh command:
```bash
virsh attach-device win2k19 device.xml
```

## Dump Debug Info
This folder ``7_dump_debug_info`` contains a debug script ``7_dump_debug_info.sh`` and a tool ``intel_gpu_top`` to dump debug info logs of Host.
The debug info contains system info, gpu info and vm info. Running the script with root privaliage and a folder ``dumplogs`` and sub-folders ``host``, ``host_config`` and ``virsh`` will be created and logs files will be saved in them.

Params:
- Use ``-h/--help`` to get the usage options of this script.
- Use ``--with-dependency`` to install the dependency for the first time. Besides, this script does not cover the installation of the ``XPU manager`` tool, and mannually installation is needed refering to [Install XPU manager](README.md).
- Use ``--gpu-card=`` to indicate the first GPU card id, default is 0. When there's an iGPU, this param need to be set as 1.

Example Commands:
```bash
cd 7_dump_debug_info
sudo ./7_dump_debug_info.sh -h
sudo ./7_dump_debug_info.sh --with-dependency
sudo ./7_dump_debug_info.sh --gpu-card=1
```

Dumped debug logs:
```
dumplogs/
├── host
│   ├── CPU_pwmon.log
│   ├── dmesg.log
│   ├── free.log
│   ├── GPUTOP-renderD128.json
│   ├── i915_engine_info_0.log
│   ├── i915_error_state_0.log
│   ├── i915_gem_objects_0.log
│   ├── journalctl.log
│   ├── ps_aux.log
│   ├── sar_dump.log
│   ├── top.log
│   └── xpum
│       ├── xpum-disc-card0.txt
│       ├── xpum-dump-card0.txt
│       ├── xpum-list.txt
│       └── xpum-stat-card0.txt
├── host_config
│   ├── cmdline.txt
│   ├── cpu_numa_info.txt
│   ├── cpu_type
│   ├── GPU-device-map.txt
│   ├── i915_driver_check.txt
│   ├── kmod_version.txt
│   ├── lscpu-p.txt
│   ├── lscpu.txt
│   ├── lspci_card_info.txt
│   ├── lspci-t.txt
│   ├── media_driver_version.txt
│   ├── numa_info.txt
│   ├── PCI.txt
│   ├── scaling_governor
│   ├── slabinfo.txt
│   └── uname-r.txt
└── virsh
    ├── virsh-VM-list.txt
    ├── virsh-win10.xml
    └── virsh-win2k19.xml
```
