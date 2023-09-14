#!/bin/bash 

PORT_VNC=10001
PORT_RDP=20001

sudo iptables -I FORWARD -m state -d 192.168.122.0/24 --state NEW,RELATED,ESTABLISHED -j ACCEPT

for vmid in $(virsh list |grep win2k19- |awk '{print $2}')
do
  IP=$(virsh domifaddr ${vmid}|grep ipv4 |awk '{print $4}' |awk -F '/' '{print $1}')
  
    sudo iptables -t nat -I PREROUTING -p tcp --dport ${PORT_VNC} -j DNAT --to ${IP}:5900
    sudo iptables -t nat -I PREROUTING -p tcp --dport ${PORT_RDP} -j DNAT --to ${IP}:3389
    PORT_VNC=$[$PORT_VNC+1]
    PORT_RDP=$[$PORT_RDP+1]
    sleep 1

done

iptables -t nat -L -n | grep -E '5900'
iptables -t nat -L -n | grep -E '3389'
