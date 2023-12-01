if [ $# = 0 ]
then
    echo "Usage: $0 vm_name mem_size mem_path mem_id pci_addr"
    exit
fi

vm_name=$1
mem_size=$2
mem_path=$3
mem_id=$4
pci_addr=$5

virsh qemu-monitor-command --hmp $vm_name "object_add memory-backend-file,size=$mem_size,share,mem-path=$mem_path,id=$mem_id"
virsh qemu-monitor-command --hmp $vm_name "device_add ivshmem-plain,memdev=$mem_id,bus=pci.0,addr=$pci_addr"
