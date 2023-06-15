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

if [ $# -lt 3 ]
then
    echo "Usage: balloon.sh monitor_dir polling_interval passive|normal|aggressive"
    exit
fi
balloon_mode=$3
polling_interval=$2
monitor_dir=$1
if (( $polling_interval <= 0 )) || (( $polling_interval >10 ))
then
    echo "Polling interval should between 0~10, use default value 2"
    polling_interval=2
fi
if [ ! -d $monitor_dir ]
then
   echo "Dir $monitor_dir not found!" >&2
   exit
fi
if [ $balloon_mode == 'passive' ]
then
    let memory_div=400
elif [ $balloon_mode == 'normal' ]
then
    let memory_div=200
elif [ $balloon_mode == 'aggressive' ]
then
    let memory_div=125
else
    echo "Usage: balloon.sh monitor_dir polling_interval passive|normal|aggressive"
    exit
fi
memory_div=`echo "scale=2;$memory_div/100" |bc`
awk ' !x[$0]++' mac.txt > rmv_dup_mac.txt   #remove duplicate mac address from mac.txt
while read line
do
    array=($line)
    maclist[$i]=${array[0]}
    let i++
done<rmv_dup_mac.txt
init_str1="\{\\\"execute\\\":\\\"qmp_capabilities\\\"\}\r"
balloon_polling="\{\\\"execute\\\": \\\"qom-set\\\",\\\"arguments\\\": \{ \\\"path\\\": \\\"\/machine\/peripheral\/balloon0\\\",\\\"property\\\": \\\"guest-stats-polling-interval\\\", \\\"value\\\": $polling_interval \}\}\r"
balloon_status="\{\\\"execute\\\": \\\"qom-get\\\",\\\"arguments\\\": \{ \\\"path\\\": \\\"\/machine\/peripheral\/balloon0\\\",\\\"property\\\": \\\"guest-stats\\\"\}\}\r"
echo $init_str1
echo $balloon_status
min_free_mem=`expr 500*1024*1024`
while :
do
for vm_mac in ${maclist[@]};do
    expect <<EOF
    set timeout 5
    spawn nc -U $monitor_dir/testvm$vm_mac.monitor
    log_file
    expect "oob" { send "$init_str1" }
    expect "return" { send "$balloon_polling" }
    sleep $polling_interval
    expect "return" { send "$balloon_status" }
    log_file -noappend $vm_mac.status
    expect "last-update" { send "\003" }
    expect eof
EOF
    array=`cat $vm_mac.status`
    free_mem=`echo ${array#*stat-free-memory} | awk -F ":" '{print $2}' | awk -F"," '{print $1}'`
    total_mem=`echo ${array#*stat-total-memory} | awk -F ":" '{print $2}' | awk -F"," '{print $1}'`
    if [ "$free_mem" -gt 0 ] 2>/dev/null ;then 
        echo "$free_mem" 
    else 
        free_mem=0
    fi
    if [ "$total_mem" -gt 0 ] 2>/dev/null ;then
        echo "$total_mem"
    else
        let total_mem=`expr 4096*1024*1024`
    fi
    if (( $free_mem < $min_free_mem ))  #500M is threshhold
    then
        release_mem=0
    else
        release_mem=`echo "scale=0;$free_mem/$memory_div" |bc`
    fi
    rm -f $vm_mac.status
    balloon_mem=$(($total_mem-$release_mem))
    echo $free_mem
    echo $total_mem
    echo $balloon_mem
    set_balloon_str="\{\\\"execute\\\": \\\"balloon\\\", \\\"arguments\\\": \{ \\\"value\\\": $balloon_mem \}\}\r"
    expect <<EOF
    set timeout 5
    spawn nc -U $monitor_dir/testvm$vm_mac.monitor
    expect "oob" { send "$init_str1" }
    expect "return" { send "$set_balloon_str" }
    expect {
        "BALLOON_CHANGE" { send "\003" }
        "error" { send "\003" }
    }
    expect eof
EOF
done
done
