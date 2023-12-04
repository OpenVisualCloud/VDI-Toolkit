#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import sys
import getopt
import subprocess
import time
iplist=[]      
processdict={}
ipfile="vm_ip.txt"
rdp_processdict = {}
def find_vm_ip():
    global iplist
    global ipfile
    for i in open(ipfile):
        ip=i.rstrip('\n')
        iplist.append(ip)

def help():
    print """
    Usage: vmtestrun [options]"
    \t-u: user name
    \t-p: password
    \t-c: connection type RDP|VNC [default : RDP]
    \t-f: iplist file  [default : vm_ip.txt]
    """
            
def main():
    global iplist
    global ipfile
    connect_type = "RDP"
    username = ""
    password = ""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hp:c:u:f:")
    except getopt.GetoptError:
        help()
    for opt, arg in opts:
        if opt == "-h":
            help()
            sys.exit()
        elif opt == "-p":
            password = opt+arg
        elif opt == "-c":
            connect_type = arg
        elif opt == "-u":
            username = opt+arg
        elif opt == "-f":
            ipfile = arg
            
    find_vm_ip()
    systype=sys.platform
    print(systype)
    print(iplist)
    if systype == "linux" or systype == "linux2":
        if connect_type == "RDP":
            connect_py = "/usr/bin/rdpy-rdpclient.py"
        else:
            connect_py = "/usr/bin/rdpy-vncclient.py"
            username=""
    else:
        if connect_type == "RDP":
            connect_py = "c:/python27/scripts/rdpy-rdpclient.py"
        else:
            connect_py = "c:/python27/scripts/rdpy-vncclient.py"
            username=""
    if username =="" and password =="":
        connect_py =""
    for vm_ip in iplist:
        print(vm_ip)
        print(systype)
        if systype == "linux" or systype == "linux2":
            p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip])
        else:
            p=subprocess.Popen(["python", "winapptest.py",vm_ip],shell=False)
        processdict[vm_ip] = p;
        time.sleep(0.05)
        if len(connect_py) != 0:
            if systype == "linux" or systype == "linux2":
                p=subprocess.Popen(["python2.7", connect_py,username,password,vm_ip])
            else:
                if len(username) != 0:
                    p=subprocess.Popen(["python", connect_py,username,password,vm_ip],shell=True)
                else:
                    p=subprocess.Popen(["python", connect_py,password,vm_ip],shell=True)
            rdp_processdict[vm_ip] = p
            time.sleep(0.05)

    while True:
        i=0
        pid_dict={}
        for vm_ip in iplist:
            p = processdict[vm_ip]
            proccess_str = "vmip {} process status {}"
            status = p.poll()
            if status == None:
                  print(proccess_str.format(vm_ip," OK"))
                  print("process pid %d" % p.pid)
            else:
                  print(proccess_str.format(vm_ip,p.returncode))
                  if systype == "linux" or systype == "linux2":
                      p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip])
                  else:
                      p=subprocess.Popen(["python", "winapptest.py",vm_ip],shell=False)
                  processdict[vm_ip] = p

            pid_dict[vm_ip] = p.pid
            time.sleep(2)
            if len(connect_py) != 0:
                proccess_str="vmip {} {} process status {}"
                p = rdp_processdict.get(vm_ip)
                if p:
                    status = p.poll()
                else:
                    status = None
                if status == None:
                    print(proccess_str.format(vm_ip,connect_type," OK"))
                else:
                    print(proccess_str.format(vm_ip,connect_type,p.returncode))

                    if systype == "linux" or systype == "linux2":
                        p=subprocess.Popen(["python2.7",connect_py,username,password,vm_ip])
                    else:
                        p=subprocess.Popen(["python", connect_py,username,password,vm_ip],shell=True)
                    rdp_processdict[vm_ip] = p
                time.sleep(2)
            i=i+1
        listfile = open('plist.txt', 'w')
        for  ip,pid in  pid_dict.items():
            listfile.write(ip+','+str(pid)+'\n')
        listfile.close()
    if systype == "linux" or systype == "linux2":
        testinput=input()
    else:
        os.system('pause')

if __name__ == '__main__':
    main()

