if [ $# = 0 ]
then
    echo "Usage: $0 vm_name device_id serial_path serial_id port_name"
    exit
fi

vm_name=$1
device_id=$2
serial_path=$3
serial_id=$4
port_name=$5

virsh qemu-monitor-command --hmp $vm_name "device_add virtio-serial-pci,id=$device_id"
virsh qemu-monitor-command --hmp $vm_name "chardev-add socket,path=$serial_path,server,nowait,id=$serial_id"
virsh qemu-monitor-command --hmp $vm_name "device_add virtserialport,chardev=$serial_id,name=$port_name"
