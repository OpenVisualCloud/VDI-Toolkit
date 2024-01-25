#!/bin/bash
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

if [ $# -lt 1 ]
then
    echo "Usage: findip.sh nat|bridge"
    exit
fi
network_mode=$1
temp_file_name=$0.temp
temp_out_file=$0.out

if [ "$network_mode" == 'nat' ]
then
    segment='172.16.10'
    min_seg=0
    max_seg=1
else
    segment=$(ifconfig br0 | grep inet | grep -v inet6|awk '{print $2}'|tr -d "addr:"|awk -F. '{print $1 "." $2 "." $3}')
    min_seg=-1
    max_seg=1
fi
echo "$segment"
i=0
awk ' !x[$0]++' mac.txt > rmv_dup_mac.txt   #remove duplicate mac address from mac.txt
while read -r line
do
    array=("$line")
    maclist[i]=${array[0]}
    (( i++ ))
done<rmv_dup_mac.txt
length=${#maclist[@]}
echo "$length"
for((range=min_seg;range<=max_seg;range++));
do
 segment1=$(echo "$segment"|awk -F. '{print $1 "." $2 "." ($3+'"$range"')}')
 echo "$segment1"
 for i in  {1..254}
  do
      {
        ping -c2 -W1 "${segment1}"."${i}"  &>/dev/null
          # shellcheck disable=SC2181
        if [ $? -eq 0 ]; then
           iplist+=("${segment1}"."$i")
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
    for vm_mac in "${maclist[@]}";do
        for ip_mac in "${arplist[@]}";do
            ip=${ip_mac%/*}
            mac=${ip_mac#*/}
            if [ ${#mac} -eq 17 ] || [ "${mac}" != 'ff-ff-ff-ff-ff-ff' ]
            then
                if [ "$vm_mac" == "$mac" ];then
                    echo "$ip" >>vm_ip.txt
                    echo "$mac" "$ip" >>mac_ip.txt
                fi
            else
                echo "no match ip for $mac"
            fi
        done
    done
        
    
}
 rm -f "$temp_file_name"
 rm -f "$temp_out_file"
 arp -a > "$temp_file_name"
# shellcheck disable=SC2002
 cat "$temp_file_name" | awk '{split($0,ip,"[()]");printf ip[2] " ";a=index($0," at ");b=index($0," on ");print substr($0,a+4,b-a-4)}' > "$temp_out_file"
 echo "IP-MAC map to $temp_out_file"
 rm -f vm_ip.txt
 rm -f mac_ip.txt
 i=0
 while read -r line
 do
    array=("$line")
    arplist[i]=${array[0]}"/"${array[1]}
    (( i++ ))
 done<"$temp_out_file"
# shellcheck disable=SC2128
 handle_ip_mac "$arplist"

