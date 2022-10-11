#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import subprocess
import time
iplist=[]      
      
def find_vm_ip():
    global iplist
    for i in open("vm_ip.txt"):
        ip=i.rstrip('\n')
        iplist.append(ip)
            
def main():
    global iplist
    find_vm_ip()
    print iplist
    for vm_ip in iplist:
        print vm_ip
        subprocess.Popen(["python", "winapptest.py",vm_ip],shell=True)
        time.sleep(0.05)
    os.system('pause')

if __name__ == '__main__':
    main()

