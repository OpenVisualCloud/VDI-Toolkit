#!/bin/sh
#******************************************************************************
#
# Copyright (c) 2023 Intel Corporation. All rights reserved.
#
# This code is licensed under the MIT License (MIT).
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
#******************************************************************************

if [ $# -lt 6 ]
then
    echo "Usage: createvm.sh original_qcow2_name dest_dir start_vm vm_count cpu_count memory_size nat|bridge"
    exit
fi
network_mode='nat'
if [ $# -eq 7 ] 
then 
    network_mode=$7
fi
let start_vm=$3
let vm_count=$4
let cpu_count=$5
let memory_size=$6
dest_dir=$2
org_image=$1
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
    qemu-kvm -drive file=$dest_dir/testvm$i.qcow2,format=qcow2,if=virtio,aio=native,cache=none -m $memory_size -smp $cpu_count -M q35 -cpu host,host-cache-info=on,migratable=on,hv-time=on,hv-relaxed=on,hv-vapic=on,hv-spinlocks=0x1fff  -enable-kvm -display none -netdev tap,id=mynet$i,vhost=on,queues=2,ifname=tap$i,script=no,downscript=no -device virtio-net-pci,mq=on,packed=on,netdev=mynet$i,vectors=6,mac=52:55:00:d1:$mac1:$mac2 -device virtio-balloon-pci,id=balloon0 -chardev socket,id=charmonitor,path=$dest_dir/testvm52:55:00:d1:$mac1:$mac2.monitor,server,nowait -mon chardev=charmonitor,id=monitor,mode=control $vnc_setting -machine usb=on -device usb-tablet &
    echo 52:55:00:d1:$mac1:$mac2 >>mac.txt
done
# wait for all taps ready
while :
do
  sleep 1
  ifconfig tap$start_vm up
  if [ $? -eq 0 ]
  then
      break 
  fi
done
for ((i=$start_vm; i<(($start_vm+$vm_count)); i++)); do
    ifconfig tap$i up
    brctl addif $bridge_type tap$i
done

