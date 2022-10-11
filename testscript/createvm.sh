#!/bin/sh

if [ $# -lt 4 ]
then
    echo "Usage: createvm.sh original_qcow2_name dest_dir start_vm vm_count nat|bridge"
    exit
fi
network_mode='nat'
if [ $# -eq 5 ] 
then 
    network_mode=$5
fi
let start_vm=$3
let vm_count=$4
dest_dir=$2
org_image=$1
if (( $start_vm < 0 )) || (( $start_vm > 254 ))
then
    echo "start_vm should between 0~254"
    exit
fi
if (( $vm_count < 1 )) || (( $vm_count > 254 ))
then
    echo "vmcount should between 1~254"
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
    echo "Copy $org_image  to $dest_dir/base.qcow2 ..."
    if [ ! -f $dest_dir/base.qcow2 ] 
    then
        cp $org_image  $dest_dir/base.qcow2
    else
        rm -f $dest_dir/base.qcow2
        cp $org_image  $dest_dir/base.qcow2
    fi
    echo "Done"
   
    for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
    echo "Create increment file $dest_dir/testvm${i}.qcow2..."
    if [ ! -f $dest_dir/testvm${i}.qcow2 ]
    then
        qemu-img create -b $dest_dir/base.qcow2 -f qcow2 $dest_dir/testvm${i}.qcow2
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
    ifconfig br1 192.168.1.1 netmask 255.255.255.0 broadcast 192.168.1.255
    ifconfig br1 up
    iptables -t nat -A POSTROUTING -s 192.168.1.0/24 ! -d 192.168.1.0/24 -j MASQUERADE
    sysctl -w net.ipv4.ip_forward=1
    iptables -A FORWARD -s 192.168.1.0/24 -j ACCEPT
    if [ ! -f dnsmasq_vdi.conf ] 
    then
        echo "listen-address=192.168.1.1" >>dnsmasq_vdi.conf
        echo "except-interface=l0" >>dnsmasq_vdi.conf
        echo "interface=br1" >>dnsmasq_vdi.conf
        echo "bind-interfaces" >>dnsmasq_vdi.conf
        echo "dhcp-range=192.168.1.2,192.168.1.254" >>dnsmasq_vdi.conf
        echo "dhcp-no-override" >>dnsmasq_vdi.conf
    fi
    /usr/sbin/dnsmasq --conf-file=dnsmasq_vdi.conf
    sleep 1
    bridge_type=br1
else
    bridge_type=br0
fi
#create tap
for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
  tunctl -t tap$i
  brctl addif $bridge_type tap$i
  ifconfig tap$i up
done
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
for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
    if (($i >= 254 && $prev_i < 254))
    then
        ((mac1++))
    fi 
    let prev_i=$i
    mac2=$(($i % 254))
    ((mac2++))
    mac2=$(dec2hex $mac2)
    if (($i <= 99 ))
    then
        vnc_setting="-vnc 0.0.0.0:"$i
    else
        vnc_setting=""
    fi
    qemu-kvm -drive file=/nvme/testvm$i.qcow2,format=qcow2,if=virtio,aio=native,cache=none -m 4096 -smp 2 -M q35 -cpu host,host-cache-info=on,migratable=on,hv-time=on,hv-relaxed=on,hv-vapic=on,hv-spinlocks=0x1fff  -enable-kvm -display none -netdev tap,id=mynet$i,vhost=on,ifname=tap$i,script=no,downscript=no -device virtio-net-pci,mq=on,netdev=mynet$i,mac=52:55:00:d1:$mac1:$mac2 $vnc_setting -machine usb=on -device usb-tablet &
    echo 52:55:00:d1:$mac1:$mac2 >>mac.txt
done
