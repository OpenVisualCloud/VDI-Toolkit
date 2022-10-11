#!/bin/sh

if [ $# -lt 3 ]
then
    echo "Usage: createvm.sh original_qcow2_name dest_dir vm_count nat|bridge"
    exit
fi
network_mode='nat'
if [ $# -eq 4 ] 
then 
    network_mode=$4
fi
vm_count=$3
dest_dir=$2
org_image=$1
if (( $vm_count < 1 )) || (( $vm_count > 200 ))
then
    echo "vmcount should between 1~200"
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
#copy image
for ((i=0; i<$vm_count; i++)); do
    echo "Copy to /nvme/testvm${i}.qcow2..."
    if [ ! -f /nvme/testvm${i}.qcow2 ]
    then
        cp $org_image  /nvme/testvm${i}.qcow2
    fi
    echo "Done"
done

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
for ((i=0; i<$vm_count; i++)); do
  tunctl -t tap$i
  brctl addif $bridge_type tap$i
  ifconfig tap$i up
done
dec2hex(){
    printf "%02x" $1
}
# launch vm
mac1=57
mac2=0
rm -f mac.txt
for ((i=0; i<$vm_count; i++)); do
    if (($i > 254 ))
    then
        ((mac1++))
    fi 
    mac2=$(($i % 254))
    ((mac2++))
    mac2=$(dec2hex $mac2)
    qemu-kvm -drive file=/nvme/testvm$i.qcow2,format=qcow2,if=virtio,aio=native,cache=none -m 4096 -smp 2 -M q35 -cpu host -enable-kvm -netdev tap,id=mynet$i,ifname=tap$i,script=no,downscript=no -device e1000,netdev=mynet$i,mac=52:55:00:d1:$mac1:$mac2 &
    echo 52:55:00:d1:$mac1:$mac2 >>mac.txt
done
