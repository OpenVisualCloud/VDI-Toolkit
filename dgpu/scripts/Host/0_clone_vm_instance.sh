#!/bin/bash 

if [ $# = 0 ]
then
	echo "Usage: $0 qcow_dir count"
	exit
fi

qcow_dir=$1
count=$2

if [ ! -d "$qcow_dir" ]
then
	echo "Dir $qcow_dir not found!" >&2
	exit
fi

for ((i=1; i<count+1; i++));
do
	# echo "Creating VM ${i}..."
	# virsh destroy win2k19-$i
	# virt-clone -o name_of_current_vm -n name_of_new_vm -f /data2/KVM/name_of_new_vm_d1.raw
	vmid=$(virsh list |grep win2k19- |awk '{print $2}')
	result=$(echo "$vmid" |grep win2k19-"$i")
	if [ "$result" == "" ]; then
	   virt-clone -o win2k19 -n win2k19-"$i" -f "$qcow_dir"/win2k19-"$i".qcow2
	   echo "VM $i done"
	else
		echo "VM $i instance exists."
	fi
	echo "VM $i done"
done
