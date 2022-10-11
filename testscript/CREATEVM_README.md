# Use createvm.sh to launch the virtual machines as the limit of the system resource
## Usage: createvm.sh original_qcow2_name dest_dir start_vm vm_count nat|bridge
original_qcow2_name is the original qcow2 to be used for VMs
dest_dir is the destination directory to used for newly created VM images
start_vm is the start index for test VMs and also used for start mac address for test VMs
vm_count is the VM counts that need create
nat or bridge mode: nat will use 192.168.1.x address range for VMs, bridge mode will use host as bridge, the VMs will get ip address from the network switch
For example:
```
./createvm.sh /nvme/win2k16_resize.qcow2 /nvme 0 100 nat 
```
### Create 100 VMs start from index 0 use original file /nvme/win2k16_resize.qcow2, use dest folder /nvme to store VM images, the VM network will use nat mode
If dest directory is not big enough, you can split the VMs use three commands:
```
./createvm.sh /nvme/win2k16_resize.qcow2 /nvme 0 100 nat
./createvm.sh /nvme/win2k16_resize.qcow2 /nvme1 100 100 nat
./createvm.sh /nvme/win2k16_resize.qcow2 /nvme2 200 100 nat
```

The above commands will create 300 vms, each folder /nvme /nvme1 /nvme2 will hold 100 images 
After created and launched the vms, you can use findip.sh to find the vms' IP address, after executed findip.sh
the script will generate a vm_ip.txt file contains all the VMs' ip address 
```
./findip.sh nat
```
Note: to make sure the clean test result, make sure to delete all the files in dest directory, also remove mac.txt  