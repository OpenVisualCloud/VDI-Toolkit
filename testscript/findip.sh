#!/bin/sh
if [ $# -lt 1 ]
then
    echo "Usage: findip.sh nat|bridge"
    exit
fi
network_mode=$1
temp_file_name=$0.temp
temp_out_file=$0.out

if [ $network_mode == 'nat' ] 
then
    segment='192.168.1'
    min_seg=0
    max_seg=0
else
    segment=`ifconfig br0 | grep inet | grep -v inet6|awk '{print $2}'|tr -d "addr:"|awk -F. '{print $1 "." $2 "." $3}'`
    min_seg=-1
    max_seg=1
fi
echo $segment
i=0
while read line
do
    array=($line)
    maclist[$i]=${array[0]}
    let i++
done<mac.txt
length=${#maclist[@]}
echo $length
for((range=$min_seg;range<=$max_seg;range++));
do
 segment1=`echo $segment|awk -F. '{print $1 "." $2 "." ($3+'"$range"')}'`
 echo $segment1
 for i in  {1..254}
  do
      {
        ping -c2 -W1 ${segment1}.${i}  &>/dev/null
        if [ $? -eq 0 ]; then
           iplist+=(${segment1}.$i)
           echo "${segment1}.$i" &>/dev/null  >> ip-up.txt
        else
           echo "${segment1}.$i" &>/dev/null >> ip-down.txt
        fi
     }&
  done
  sleep .5
done
 function handle_ip_mac(){
    arplist=$1;
    for vm_mac in ${maclist[@]};do
        for ip_mac in ${arplist[@]};do
            ip=${ip_mac%/*}
            mac=${ip_mac#*/}
            if [ ${#mac} -eq 17 ] || [ ${mac} != 'ff-ff-ff-ff-ff-ff' ]
            then
                if [ $vm_mac == $mac ];then
                    echo $ip >>vm_ip.txt
                    echo $mac $ip >>mac_ip.txt
                fi
            else
                echo "no match ip for $mac"
            fi
        done
    done
        
    
}
 arp -a > $temp_file_name
 cat $temp_file_name | awk '{split($0,ip,"[()]");printf ip[2] " ";a=index($0," at ");b=index($0," on ");print substr($0,a+4,b-a-4)}' > "$temp_out_file"
 echo "IP-MAC map to $temp_out_file"
 rm -f vm_ip.txt
 rm -f mac_ip.txt
 i=0
 while read line
 do
    array=($line)
    arplist[$i]=${array[0]}"/"${array[1]}
    let i++
 done<$temp_out_file

 handle_ip_mac $arplist


