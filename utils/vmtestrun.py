#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import sys
import subprocess
import time
iplist=[]      
processlist=[]      
def find_vm_ip():
    global iplist
    for i in open("vm_ip.txt"):
        ip=i.rstrip('\n')
        iplist.append(ip)
            
def main():
    global iplist
    find_vm_ip()
    systype=sys.platform
    print(systype)
    print(iplist)
    for vm_ip in iplist:
        print vm_ip
        if systype == "linux2":
            p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip])
        else:
            p=subprocess.Popen(["python", "winapptest.py",vm_ip],shell=True)
        processlist.append(p); 
        time.sleep(0.05)
    while True:
        i=0
        for vm_ip in iplist:
            p=processlist[i]
            str="vmip {} process status {}"
            status=p.poll()
            if status == None:
            	  print(str.format(vm_ip," OK"))
            else:
            	  print(str.format(vm_ip,p.returncode))
            	  processlist.pop(i)
                  if systype == "linux2":
                      p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip])
                  else:
                      p=subprocess.Popen(["python", "winapptest.py",vm_ip],shell=True)
                  processlist.insert(i,p); 
            i=i+1
            time.sleep(2)
    if systype == "linux2":
        testinput=input()
    else:
        os.system('pause')

if __name__ == '__main__':
    main()

