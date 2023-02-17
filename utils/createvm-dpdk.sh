#!/bin/sh

if [ $# -lt 7 ]
then
    echo "Usage: createvm.sh original_qcow2_name dest_dir start_vm vm_count cpu_count memory_size vfio-interface nat|bridge"
    echo "vfio-interface is the NIC name in ifconfig list"
    exit
fi
network_mode='nat'
if [ $# -eq 8 ] 
then 
    network_mode=$8
fi
vfio_dev=$7
let start_vm=$3
let vm_count=$4
let cpu_count=$5
let memory_size=$6
dest_dir=$2
org_image=$1
ifconfig $vfio_dev up
if [ $? -ne 0 ]
then
    echo "Please select correct vfio_dev"
    exit
fi

pci_slot_name=`cat /sys/class/net/$vfio_dev/device/uevent | grep PCI_SLOT_NAME`
pci_slot_name=${pci_slot_name#*=}
echo $pci_slot_name

if (( $start_vm < 0 )) || (( $start_vm > 254 ))
then
    echo "start_vm should between 0~254"
    exit
fi
if (( $vm_count < 1 )) || (( $vm_count > 500 ))
then
    echo "vmcount should between 1~500"
    exit
fi
if [ ! -f $org_image ]   
then
   echo "File $org_image not found!" >&2
   exit
fi

if [ ! -d $dest_dir ]
then
   echo "Dir $dest_dir not found!" >&2
   exit
fi

if (( $cpu_count < 1 )) || (( $cpu_count > 32 ))
then
    echo "cpu count should between 1~32"
    exit
fi

if (( $memory_size < 2048 )) || (( $memory_size > 32768 ))
then
    echo "memory size  should between 2048~32768"
    exit
fi

if [ $network_mode == 'nat' ] 
then
   echo "nat mode"
elif [ $network_mode == 'bridge' ] 
then
   echo "bridge mode"
else
   network_mode='nat'
   echo "use default nat mode"
fi


image_type='increment'
#copy image
if [ $image_type == 'increment' ]
then
    echo "Copy $org_image  to $dest_dir/base$start_vm-$vm_count.qcow2 ..."
    if [ ! -f $dest_dir/base$start_vm-$vm_count.qcow2 ] 
    then
        cp $org_image  $dest_dir/base$start_vm-$vm_count.qcow2
    else
        rm -f $dest_dir/base$start_vm-$vm_count.qcow2
        cp $org_image  $dest_dir/base$start_vm-$vm_count.qcow2
    fi
    echo "Done"
   
    for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
    echo "Create increment file $dest_dir/testvm${i}.qcow2..."
    if [ ! -f $dest_dir/testvm${i}.qcow2 ]
    then
        qemu-img create -b $dest_dir/base$start_vm-$vm_count.qcow2 -f qcow2 $dest_dir/testvm${i}.qcow2
    else
        echo "Increment file $dest_dir/testvm${i}.qcow2 exist"
    fi
    echo "Done"
    done
else
    max_diff=`expr 1*1024*1024`            # original image and dest image max difference size is 1G bytes
    for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
        echo "Copy to $dest_dir/testvm${i}.qcow2..."
        if [ ! -f $dest_dir/testvm${i}.qcow2 ]
        then
            cp $org_image  $dest_dir/testvm${i}.qcow2
        else
            dest_size=`ls -s $dest_dir/testvm${i}.qcow2|awk '{print $1}'`
            org_size=`ls -s $org_image|awk '{print $1}'`
            diff_size=`expr $dest_size - $org_size`
            diff_size=${diff_size#-}
            echo $dest_size $org_size $diff_size
            if (($diff_size >  $max_diff)) 
            then
                echo "Dest file size not same as org, rm dest file..."
                rm -f $dest_dir/testvm${i}.qcow2
                cp $org_image  $dest_dir/testvm${i}.qcow
            fi
        fi
        echo "Done"
    done
fi

if [ $network_mode == 'nat' ] 
then
    brctl addbr br1
    ifconfig br1 172.16.10.1 netmask 255.255.254.0 broadcast 172.16.11.255
    ifconfig br1 up
    iptables -t nat -A POSTROUTING -s 172.16.10.0/23 ! -d 172.16.10.0/23 -j MASQUERADE
    sysctl -w net.ipv4.ip_forward=1
    iptables -A FORWARD -s 172.16.10.0/23 -j ACCEPT
    if [  -f dnsmasq_vdi.conf ] 
    then
        rm -f dnsmasq_vdi.conf
    fi
    echo "listen-address=172.16.10.1" >>dnsmasq_vdi.conf
    echo "except-interface=l0" >>dnsmasq_vdi.conf
    echo "interface=br1" >>dnsmasq_vdi.conf
    echo "bind-interfaces" >>dnsmasq_vdi.conf
    echo "dhcp-range=172.16.10.2,172.16.11.254,255.255.254.0" >>dnsmasq_vdi.conf
    echo "dhcp-no-override" >>dnsmasq_vdi.conf
    killall dnsmasq
    sleep 1
    brctl addif br1 ens21f1
    /usr/sbin/dnsmasq --conf-file=dnsmasq_vdi.conf
    sleep 1
    bridge_type=br1
else
    bridge_type=br0
fi
dec2hex(){
    printf "%02x" $1
}
# launch vm
mac2=$start_vm
mac1=55

#rm -f mac.txt
prev_i=0
local_ip=`ifconfig br0 | grep inet | grep -v inet6|awk '{print $2}'|tr -d "addr:"|awk -F. '{print $1 "." $2 "." $3 "." $4}'`
echo $local_ip

modprobe vfio-pci
/usr/bin/chmod a+x /dev/vfio
/usr/bin/chmod 0666 /dev/vfio/*
/home/media/dpdk/dpdk-stable-21.11.2/usertools/dpdk-devbind.py --bind=vfio-pci $vfio_dev
rm /usr/local/etc/openvswitch/conf.db
export LD_LIBRARY_PATH=/usr/local/lib64
ovsdb-tool create /usr/local/etc/openvswitch/conf.db  /usr/local/share/openvswitch/vswitch.ovsschema
ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock  \
--remote=db:Open_vSwitch,Open_vSwitch,manager_options  \
--private-key=db:Open_vSwitch,SSL,private_key \
--certificate=Open_vSwitch,SSL,certificate \
--bootstrap-ca-cert=db:Open_vSwitch,SSL,ca_cert --pidfile --detach
ovs-vsctl --no-wait init
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra="--no-huge -m 4096"
ovs-vswitchd --pidfile --detach --log-file
ovs-vsctl set Open_vSwitch . other_config:pmd-cpu-mask=0xc
ovs-vsctl set Interface dpdk-p0 options:n_rxq=2
ovs-vsctl add-br br2 -- set bridge br2 datapath_type=netdev
ovs-vsctl add-port br2 dpdk-p0 -- set Interface dpdk-p0 type=dpdk options:dpdk-devargs=$pci_slot_name

for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
    if (($i >= 254 && $prev_i < 254))
    then
        ((mac1++))
    fi 
    let prev_i=$i
    mac2=$(($i % 254))
    ((mac2++))
    mac2=$(dec2hex $mac2)
    vnc_setting="-vnc 0.0.0.0:"$i
    ovs-vsctl add-port br2 vhost-user$i -- set Interface vhost-user$i type=dpdkvhostuser
    qemu-kvm -drive file=$dest_dir/testvm$i.qcow2,format=qcow2,if=virtio,aio=native,cache=none -m $memory_size -smp $cpu_count -M q35 -cpu host,host-cache-info=on,migratable=on,hv-time=on,hv-relaxed=on,hv-vapic=on,hv-spinlocks=0x1fff  -enable-kvm -object memory-backend-file,id=mem,size=$memory_size'M',mem-path=/nvme1,share=on -numa node,memdev=mem -mem-prealloc  -display none -chardev socket,id=char$i,path=/usr/local/var/run/openvswitch/vhost-user$i -netdev type=vhost-user,id=mynet$i,chardev=char$i,vhostforce,queues=2 -device virtio-net-pci,netdev=mynet$i,mq=on,vectors=6,mac=52:55:00:d1:$mac1:$mac2 -device virtio-balloon-pci,id=balloon0 -chardev socket,id=charmonitor,path=$dest_dir/testvm52:55:00:d1:$mac1:$mac2.monitor,server,nowait -mon chardev=charmonitor,id=monitor,mode=control $vnc_setting -machine usb=on -device usb-tablet &
   echo 52:55:00:d1:$mac1:$mac2 >>mac.txt
done

