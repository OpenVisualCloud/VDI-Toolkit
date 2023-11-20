### CreateVM
CreateVM is the shell scripts to create up to 512 VMs using tuned parameters and user selected parameters. It will create each VM with monitor support and VNC support. It will initialize basic network environment and launch dnsmasq dhcp server to allocate each VM with dedicate IP address if user select NAT mode, if user select bridge mode, the VM’s IP address will get from bridged network

### createvm-dpdk
 This is the script to create VMs with DPDK network support
### createvm-sriov
 This is the script to create VMs with SRIOV network configuration support
 Please refer to the detailed usage guide in [CREATEVM_README](../doc/CREATEVM_README.md)

### Windows Application Driver
Windows Application Driver(WinAppDriver) is a service to support Selenium-like UI Test Automation on Windows Applications. It is open source in github. Please refer to https://github.com/microsoft/WinAppDriver for details
### Winapptest
Winapptest is a Python script to parse the test xml file and to utilize the Selenium Python library as client to connect with WinAppDriver to do auto test through the xml control.

### VMtestrun
VMtestrun is the script to run the xml file to all VMs, it will first parse the vm_ip.txt that include all VM’s ip list, and launch winapptest for each VM in parallel, then scan xml in current directory, execute the xml one by one endlessly to all VMs
Please refer to the detailed usage guide in [VMTESTRUN_README](../doc/VMTESTRUN_README.md)
### VCPUBind
VCPUBind is a utility to bind VM’ vcpu to specified logical CPU. The bindvcpu will parse vcpu bind xml file and connect to the VM’s monitor interface get cpu thread info, and bind the cpu thread to specific CPU defined in xml file.
Please refer to the detailed usage guide in [BINDVCPU_README](../doc/BINDVCPU_README.md)
### MemoryBalloon
MemoryBalloon is a utility to do memory dynamic balloon between host and guest VM, the guest VM should install balloon driver and service first, then the host can get free memory from guest using the virt-io balloon interface, also, when the guest VM memory is not enough, the host will give back the memory through the balloon interface.
Please refer to the detailed usage guide in [BALLOON_README](../doc/BALLOON_README.md)