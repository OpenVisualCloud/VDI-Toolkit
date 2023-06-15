#!/usr/bin/env python
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

import os
import subprocess
import sys
import getopt
import time
import json
import socket
import xml.dom.minidom as minidom
from xml.dom.minidom import parse

threadlist=[]     
vcpulist=[] 
monitor_file=""
cpuset=None
RECV_MAX=16*1024

# Use qemu monitor interface, get the each vcpu thread id to threadlist
def get_vcpu_threadid(monitor_file):
    global threadlist
    
    init_cmd = "{\"execute\":\"qmp_capabilities\"}\n"
    cpu_cmd  = "{\"execute\":\"query-cpus-fast\"}\n"
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(monitor_file)
        print(client.recv(RECV_MAX).decode())
        time.sleep(0.5)
        
        client.send(init_cmd.encode())
        print(client.recv(RECV_MAX).decode())
        time.sleep(0.5)
        client.send(cpu_cmd.encode())
        line = client.recv(RECV_MAX).decode()
        print(line)
        client.close()
        cpudata = json.loads(line).get('return')
        print(cpudata)
        print(len(cpudata))
        for vcpu in cpudata:
            thread = None
            index = None
            tempdata = json.dumps(vcpu)
            vcpudata = json.loads(tempdata)
            for key,values in  vcpudata.items():
                print("%s : %s" % (key,values))
                if key == "thread-id":
                    thread = values
                elif key == "cpu-index":
                    index = values
                if thread != None and index != None: 
                    threadlist.insert(index,thread)
                    index=None
                    thread=None
    print(threadlist)

# Parse the libvirt xml config file, get the cpu binding information,total cpu binding in cpuset, each vcpu binding in vcpulist
def get_xml_vcpubinding(xml_file):
    global cpuset
    print("xml : %s" % xml_file)
    DOMTree = minidom.parse(xml_file)
    collection = DOMTree.documentElement

    try:
        vcpu_tag = collection.getElementsByTagName('vcpu')[0]
    except:
        print("no vcpu define!")
        vcpus = 0
    else:
        vcpus = int(vcpu_tag.childNodes[0].data)
        cpuset = vcpu_tag.getAttribute("cpuset")
        print("vcpus %s cpuset %s" %(vcpus,cpuset))
 
    try:
        cputune_tag = collection.getElementsByTagName('cputune')[0]
    except:
        print("no cputune define!")
    else:
        vcpupins = cputune_tag.getElementsByTagName('vcpupin')
        for vcpupin in vcpupins:
            vcpu = vcpupin.getAttribute("vcpu")
            cpu =  vcpupin.getAttribute("cpuset")
            print("%s --> %s" %(vcpu,cpu))
            vcpulist.insert(int(vcpu),cpu)
    print(vcpulist)


# Create cgroup, bind the vcpu use information from threadlist and vcpulist
def final_binding():
    global monitor_file
    global cpuset
    vcpus = len(threadlist)
    cpus = len(vcpulist)
    if vcpus >= cpus:
        cpurange = cpus
    else:
        cpurange = vcpus
    if cpurange <= 0:
         return
    threadstatus="cat /proc/{0}/status | grep Tgid".format(threadlist[0])
    cpuset_dir = "/sys/fs/cgroup/cpuset/" + monitor_file.split('/')[-1]
    print(threadstatus)
    
    piddata = subprocess.run(threadstatus,shell=True,stdout=subprocess.PIPE)
    pid = piddata.stdout.decode()
    pid = pid.split(':')[-1]
    pid = int(pid)
    print("Qemu pid %d" % pid)
    
    mkdir="mkdir " + cpuset_dir
    print(mkdir)
    cgroupdata = subprocess.run(mkdir,shell=True,stdout=subprocess.PIPE)
    print(cgroupdata.stdout.decode())

    cpusetcmd = "echo " + cpuset  + " > " + cpuset_dir +"/cpuset.cpus"
    print(cpusetcmd)
    cgroupdata = subprocess.run(cpusetcmd,shell=True,stdout=subprocess.PIPE)
    print(cgroupdata.stdout.decode())

    memsetcmd = "echo 0 > " + cpuset_dir +"/cpuset.mems"
    print(memsetcmd)
    cgroupdata = subprocess.run(memsetcmd,shell=True,stdout=subprocess.PIPE)
    print(cgroupdata.stdout.decode())

    taskcmd = "echo {0} > {1}/tasks".format(pid,cpuset_dir)
    print(taskcmd)
    cgroupdata = subprocess.run(taskcmd,shell=True,stdout=subprocess.PIPE)
    print(cgroupdata.stdout.decode())


    for i in range(cpurange):
        vcpu_dir = "{0}/vcpu{1}".format(cpuset_dir,i)
        vcpu_mkdir = "mkdir " + vcpu_dir
        cgroupdata = subprocess.run(vcpu_mkdir,shell=True,stdout=subprocess.PIPE)
        print(cgroupdata.stdout.decode())

        vcpusetcmd = "echo {0} > {1}/cpuset.cpus".format(vcpulist[i],vcpu_dir)
        print(vcpusetcmd)
        cgroupdata = subprocess.run(vcpusetcmd,shell=True,stdout=subprocess.PIPE)
        print(cgroupdata.stdout.decode())
        
        vcpumemsetcmd = "echo 0 > {0}/cpuset.mems".format(vcpu_dir)
        print(vcpumemsetcmd)
        cgroupdata = subprocess.run(vcpumemsetcmd,shell=True,stdout=subprocess.PIPE)
        print(cgroupdata.stdout.decode())
        
        vcputaskcmd = "echo {0} > {1}/tasks".format(threadlist[i],vcpu_dir)
        print(vcputaskcmd)
        cgroupdata = subprocess.run(vcputaskcmd,shell=True,stdout=subprocess.PIPE)
        print(cgroupdata.stdout.decode())

def help():
    print ("""
    Usage: bindvcpu [options]"
    \t-m: guest monitor file name
    \t-f: vcpu binding xml file name
    """)
            
def main():
    global monitor_file
    xml_file = ""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hm:f:")
    except getopt.GetoptError:
        help()
        sys.exit()
    for opt, arg in opts:
        if opt == "-h":
            help()
            sys.exit()
        elif opt == "-m":
            monitor_file = arg
        elif opt == "-f":
            xml_file = arg
        else:
            help()
            sys.exit()    
    get_vcpu_threadid(monitor_file)
    systype=sys.platform
    print(systype)
    get_xml_vcpubinding(xml_file)
    final_binding()


if __name__ == '__main__':
    main()

