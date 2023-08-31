VF & VM setup script readme

Notes:
# The 1_vf_setup_by_xpumcli.sh script need install xpumcli tool. 


0_clone_vm_instance.sh
This script is used to clone an assigned number of qcow2 VM disk image from a source disk image. We assume that we have a well built qcow2 image already.
This script could create the assigned number of VMs instances. Usage of this script shall be (where qcow_dir is the directory of qcow images and count will be the count of the VMs.):
./0_clone_vm_instance.sh qcow_dir count


1_vf_setup_by_xpumcli.sh:
This script is used to xpumcli tool generate an assigned number of VFs from the assigned GPU.
The usage of this script shall be:
./1_vf_setup.sh card_id vf_count lmem(GB)
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

2_assign_vGPU_to_VMs.sh
This script is used to assign VF to each VM. Normally we could assign 1 VF for each VM by simply running it.


3_start_VMs.sh
This script is used to start all VMs that are generated with previous scripts. No specific params acquired.


4_vm_port_forward.sh
This script is used to map ports for all VM. VNC ports of VM will be set to 10001~10016 and RDP ports will be set to 20001~20016 according to VM IDs.
No specific params acquired.


5_shutdown_VMs.sh
This script is used to shutdown all VMs that are generated with previous scripts.


