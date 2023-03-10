# Use bindvcpu.py to bind the vm guest vcpu to host cpu
## Usage: bindvcpu.py -m vm_monitor_file -f vm_cpu_bind_xml_file
## bindvcpu.py use guest qemu monitor interface to query each vcpu thread id, and then use cgroup cpuset to bind each thread to dedicate host CPU from xml file 
## The cpubind.xml file is compatible with libvirt xml file, you can also directly use libvirt xml 
### Bind the guest vcpu:
```
 python bindvcpu.py -m testvm.monitor -f cpubind.xml
```