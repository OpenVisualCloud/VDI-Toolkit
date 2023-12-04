import sys,os
import multiprocessing
import subprocess
import time
import sqlite3
import psutil
import configparser
import json
import socket
import numpy
import math
from PIL import Image
from configparser import ConfigParser

from PyQt5 import QtCore, QtWidgets, QtGui
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *

class QMP_Query():

    def __init__(self):
        self.init_cmd = "{\"execute\":\"qmp_capabilities\"}\n"
        self.cpu_cmd  = "{\"execute\":\"query-status\"}\n"
        self.RECV_MAX = 16*1024

    def connect(self,addr,port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
            client.connect((addr,port))
            print(client.recv(self.RECV_MAX).decode())
            client.send(self.init_cmd.encode())
            print(client.recv(self.RECV_MAX).decode())
            time.sleep(0.5)
            client.send(self.cpu_cmd.encode())
            line = client.recv(self.RECV_MAX).decode()
            print(line)
            client.close()

class VMsnapshotthread(QThread):
    updatetext = pyqtSignal(list)
    def __init__(self):
        super(VMsnapshotthread, self).__init__()
        self.iplist = []
        self.listWidget = None
        self.rdp_processlist = []
        self.test_processdict ={}
        self.run_parameters = {}
        self.working = False

    def set_iplist(self,iplist):
        self.iplist = iplist

    def set_run_parameters(self,run_param):
        self.run_parameters = run_param

    def set_listWidget(self,listWidget):
        self.listWidget = listWidget

    def get_testprocess(self,listfilename):
        try:
            list_file = open(listfilename, 'r')
        except:
            print("File %s not found" % listfilename)
        else:
            for readline in list_file.readlines():
                line_list = readline.split(',')
                self.test_processdict[line_list[0]] = line_list[1].rstrip('\n')
            list_file.close()
            print(self.test_processdict)


    def update_vm_info(self,dbname,vm_ip):
        db_conn = sqlite3.connect(dbname)
        db_cur = db_conn.cursor()
        sql_text_1 = "SELECT * from "+self.run_parameters['status_table']+" where ip='" + vm_ip +"' order by rowid desc LIMIT 1;"
        print(sql_text_1)
        cursor = db_cur.execute(sql_text_1)
        co_name_list = [ tuple[0] for tuple in db_cur.description ]
        print(co_name_list)

        status = ""
        str1 = ""
        for row in cursor:
            print(row)
            i = 0
            for column in co_name_list:
                status = " %s : %s " % (column,row[i])
                str1 += status
                i += 1
        print(str1)
        param_list=[vm_ip,str1]
        self.updatetext.emit(param_list)
        db_conn.close()

    def run(self):
        if self.working == False:
            self.working = True
        else:
            return
        print("start update List ...........")
        systype=sys.platform
        connect_type = self.run_parameters['snapshot_default_type']
        username="-u " + self.run_parameters['username']
        password="-p " + self.run_parameters['password']

        taskmgr_processdict ={}
        suspend_dict = {}
        for vm_ip in self.iplist:
            if vm_ip in self.test_processdict:
                pid = int(self.test_processdict.get(vm_ip))
                try:
                    psprocess = psutil.Process(pid)
                except:
                    print("Process %d did not exist" % pid)
                else:
                    print("Process %d ip %s suspend" %(pid,vm_ip))
                    psprocess.suspend()
                    suspend_dict[vm_ip] = psprocess
            if connect_type == 'RDP':
                if systype == "linux" or systype == "linux2":
                    p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['disconnectrdp_xml']])
                else:
                    p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['disconnectrdp_xml']],shell=True)
                p.wait()

            if systype == "linux" or systype == "linux2":
                  p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['status_query_xml']])
            else:
                  p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['status_query_xml']],shell=True)
            taskmgr_processdict[vm_ip] = p

        for vm_ip,p in taskmgr_processdict.items():
            p.wait()
            psprocess = suspend_dict.get(vm_ip)
            if psprocess:
                print("Process %d ip %s resumed" %(psprocess.pid,vm_ip))
                psprocess.resume()

        if systype == "linux" or systype == "linux2":
            outdir = "-o " + os.getcwd() + "/"
            if connect_type == "RDP":
                connect_py = self.run_parameters['linux_rdp_snapshot_path']
            else:
                connect_py = self.run_parameters['linux_vnc_snapshot_path']
                username=""
                password="-p " + self.run_parameters['vnc_password']
        else:
            outdir = "-o " + os.getcwd() + "\\"
            if connect_type == "RDP":
                connect_py = self.run_parameters['win_rdp_snapshot_path']
            else:
                connect_py = self.run_parameters['win_vnc_snapshot_path']
                username=""
                password="-p " + self.run_parameters['vnc_password']

        if len(connect_py) != 0:
            for vm_ip in self.iplist:
                cmdstring="python "+connect_py+" "+username+" "+password+" "+outdir+" "+vm_ip
                if systype == "linux" or systype == "linux2":
                    p = os.popen("python2.7 %s %s %s %s %s" % (connect_py,username,password,outdir,vm_ip),"w",1)
                else:

                    if len(username) != 0:
                        p = os.popen("python %s %s %s %s %s" % (connect_py,username,password,outdir,vm_ip),"w",1)
                    else:
                        p = os.popen("python %s %s %s %s" % (connect_py,password,outdir,vm_ip),"w",1)

                self.rdp_processlist.append(p);

                self.sleep(0.05)

        for vm_ip in self.iplist:
            self.update_vm_info(self.run_parameters['dbname'],vm_ip)

        self.working = False
        print("update List ...........end")

class VMWidget(QtWidgets.QWidget):
    def __init__(self,ip,cpu_ratio,memory_ratio):
        super(VMWidget, self).__init__()

        self.widgeticon = QtWidgets.QLabel()
        icon_pix = QPixmap('default.jpg')
        self.widgeticon.setPixmap(icon_pix)
        self.widgeticon.setScaledContents(True)
        self.widgettext = QtWidgets.QLabel(ip)
        self.widgetcpu = QtWidgets.QProgressBar()
        if cpu_ratio > 0 :
            self.widgetcpu.setRange(0,100)
        else:
            self.widgetcpu.setRange(0,0)
        self.widgetcpu.setValue(cpu_ratio)
        self.widgetcpu.setAlignment(Qt.AlignRight)
        self.widgetcpu.setFormat('CPU:%p%')

        self.widgetmemory = QtWidgets.QProgressBar()
        if memory_ratio > 0:
            self.widgetmemory.setRange(0,100)
        else:
            self.widgetmemory.setRange(0,0)
        self.widgetmemory.setValue(memory_ratio)
        self.widgetmemory.setAlignment(Qt.AlignLeft)
        self.widgetmemory.setFormat('Memory:%p%')
        self.widgetlayout = QtWidgets.QVBoxLayout()
        self.widgetlayout.addWidget(self.widgeticon)
        self.widgetlayout.addWidget(self.widgettext)
        self.widgetlayout.addWidget(self.widgetcpu)
        self.widgetlayout.addWidget(self.widgetmemory)
        self.widgetlayout.addStretch()
        self.widgetlayout.setSizeConstraint(QtWidgets.QLayout.SetFixedSize)
        self.setLayout(self.widgetlayout)

class VMItem(QListWidget):
    def __init__(self):
        super(VMItem, self).__init__()
        self.setViewMode(QListView.IconMode)
        self.setResizeMode(QListView.Adjust)
        self.setMovement(QListWidget.Static)
        self.widgetdict = {}

    def add_vm_image(self,ipaddress):
        image_path= ipaddress +".jpg"

        image_path="default.jpg"
        print(image_path)
        item = QListWidgetItem(ipaddress)
        font = item.font()
        font.setPointSize(18)
        item.setFont(font)
        item.setForeground(QColor(Qt.transparent))    # just for item select window width

        self.addItem(item)
        self.widgetdict[ipaddress] = VMWidget(ipaddress,0,0)
        item.setSizeHint(self.widgetdict[ipaddress].sizeHint())
        item.setTextAlignment(Qt.AlignCenter)

        self.setItemWidget(item, self.widgetdict[ipaddress])

    def update_vm_text(self,newtext):
        oldtext = self.currentItem().text()
        newtext = oldtext +"\n" + newtext
        self.currentItem().setText(newtext)


class VMPanel(QtWidgets.QWidget):

    def __init__(self):
        super(VMPanel, self).__init__()

        self.vmsnapshotthread = VMsnapshotthread()
        self.run_parameters = {}
        self.ui_parameters = {}
        self.load_config("vmmonitor.config")
        self.initui()
        self.datas = []
        self.iplist = []
        self.rdp_processlist = []
        self.singleclicktimer=QTimer()
        self.vmupdatetimer=QTimer()
        self.singleclicktimer.setSingleShot(True)
        self.doubleclicked = False
        self.singleclicktimer.timeout.connect(self.singleclicktimeout)
        self.vmupdatetimer.timeout.connect(self.vmupdatetimer_trigger)
        self.qmp = QMP_Query()

    def initui(self):

        self.listWidget = VMItem()
        self.showLabel = QLabel(self)
        self.showLabel.setAlignment(Qt.AlignCenter )

        self.showLabel.setWordWrap(True)
        self.scrollArea = QScrollArea(self)                   # 2
        self.scrollArea.setWidget(self.showLabel)
        self.scrollArea.setWidgetResizable(False)

        self.layout = QHBoxLayout(self)
        self.layout.addWidget(self.listWidget)
        self.layout.addWidget(self.scrollArea)
        self.layout.setStretchFactor(self.listWidget, int(self.ui_parameters['stretch_factor1']))
        self.layout.setStretchFactor(self.scrollArea, int(self.ui_parameters['stretch_factor2']))
        self.create_main_menu()

        self.resize(QDesktopWidget().screenGeometry().width()*float(self.ui_parameters['desktop_width_ratio']),
                    QDesktopWidget().screenGeometry().height()*float(self.ui_parameters['desktop_height_ratio']))
        self.vmsnapshotthread.updatetext.connect(self.update_listitem_text)
        self.listWidget.itemDoubleClicked.connect(self.dbclick)
        self.listWidget.itemClicked.connect(self.click)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.generate_rightclick_menu)

    def create_main_menu(self):
        menuBar = QMenuBar(self)
        self.layout.setMenuBar(menuBar)
        self.refreshAction = QAction(self)
        self.refreshAction.setText("&Refresh")
        self.refreshAction.setCheckable(True)
        self.refreshAction.triggered.connect(self.refresh_enable)
        self.exitAction = QAction("&Exit", self)
        self.exitAction.triggered.connect(self.close)

        fileMenu = QMenu("&File", self)
        menuBar.addMenu(fileMenu)

        fileMenu.addAction(self.refreshAction)
        fileMenu.addAction(self.exitAction)
        self.menuBar = menuBar

    def close(self):
        exit()

    def refresh_enable(self):
        if self.refreshAction.isChecked():
            self.vmupdatetimer.start(int(self.run_parameters['vm_status_query_interval']))
        else:
            self.vmupdatetimer.stop()
    def load_config(self,configfile):
        config = ConfigParser()
        config.read(configfile,encoding='UTF-8')
        self.ui_parameters = dict(config.items('UI'))
        if  bool(self.ui_parameters) == False:
            print("No UI config,Config file error!")
        else:
            print(self.ui_parameters)
        self.run_parameters = dict(config.items('VM_CONFIG'))
        if  bool(self.run_parameters) == False:
            print("No run config,Config file error!")
        else:
            print(self.run_parameters)

    def update_list(self):
        for ip in self.iplist:
            self.listWidget.add_vm_image(ip)


    def vmupdatetimer_trigger(self):
        self.vmsnapshotthread.set_run_parameters(self.run_parameters)
        self.vmsnapshotthread.set_iplist(self.iplist)
        self.vmsnapshotthread.set_listWidget(self.listWidget)
        self.vmsnapshotthread.get_testprocess(self.run_parameters['process_listfile_name'])
        self.vmsnapshotthread.start()

    def update_listitem_text(self,param_list):
        for i in range(self.listWidget.count()):
            if self.listWidget.item(i).text() == param_list[0]:
                self.listWidget.item(i).setToolTip(param_list[1])
                widget = self.listWidget.widgetdict.get(param_list[0])
                if widget:
                    index = param_list[1].find("cpu ratio :") + 12  # get cpu ratio number
                    try:
                       cpu_ratio = int(param_list[1][index:index+2].replace('%',''))
                    except:
                       print("Can not get cpu ratio")
                       cpu_ratio = 0
                    widget.widgetcpu.setRange(0,100)
                    widget.widgetcpu.setValue(cpu_ratio)
                    index = param_list[1].find("memory status :") + 28  # get memory status number
                    try:
                        memory_ratio = int(param_list[1][index:index + 2].replace('%',''))
                    except:
                       print("Can not get memory ratio")
                       memory_ratio = 0
                    widget.widgetmemory.setRange(0,100)
                    widget.widgetmemory.setValue(memory_ratio)
                break
    def do_file_transfer(self,ipname):
        systype=sys.platform
        vm_ip = ipname
        if systype == "linux" or systype == "linux2":
            p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['filetransfer_xml']])
        else:
            p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['filetransfer_xml']],shell=True)

        p.wait()
    def open_file(self):
        get_filename_path, ok = QFileDialog.getOpenFileName(self,
                  "Select one file",
                  os.getcwd(),
                  "Run Once XML (*.rox)")
        if ok:
            return str(get_filename_path)
        else:
            return None

    def do_exec_xml(self,ipname):
        systype=sys.platform
        vm_ip = ipname
        filename = self.open_file()
        if filename:
            if systype == "linux" or systype == "linux2":
                p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,filename])
            else:
                p=subprocess.Popen(["python", "winapptest.py",vm_ip,filename],shell=True)

            p.wait()

    def psnr(self,img1, img2):
        m = numpy.mean((img1 - img2) ** 2)
        if m == 0:
            return 100
        logvalue = 255.0/math.sqrt(m)
        psnr = 20 * math.log10( logvalue )
        return psnr

    def do_psnr(self,ipname):
        systype=sys.platform
        connect_type = self.run_parameters['snapshot_default_type']
        username="-u " + self.run_parameters['username']
        password="-p " + self.run_parameters['password']
        vm_ip = ipname
        if systype == "linux" or systype == "linux2":
            p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['filetransfer_xml']])
        else:
            p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['filetransfer_xml']],shell=True)

        p.wait()
        if connect_type == 'RDP':
            if systype == "linux" or systype == "linux2":
                p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['disconnectrdp_xml']])
            else:
                p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['disconnectrdp_xml']],shell=True)

            p.wait()

        if systype == "linux" or systype == "linux2":
            p=subprocess.Popen(["python2.7", "winapptest.py",vm_ip,self.run_parameters['psnr_xml']])
        else:
            p=subprocess.Popen(["python", "winapptest.py",vm_ip,self.run_parameters['psnr_xml']],shell=True)

        p.wait()

        if systype == "linux" or systype == "linux2":
            outdir = "-o " + os.getcwd() + "/"
            if connect_type == "RDP":
                connect_py = self.run_parameters['linux_rdp_snapshot_path']
            else:
                connect_py = self.run_parameters['linux_vnc_snapshot_path']
                username=""
                password="-p " + self.run_parameters['vnc_password']
        else:
            outdir = "-o " + os.getcwd() + "\\"
            if connect_type == "RDP":
                connect_py = self.run_parameters['win_rdp_snapshot_path']
            else:
                connect_py = self.run_parameters['win_vnc_snapshot_path']
                username=""
                password="-p " + self.run_parameters['vnc_password']

        if len(connect_py) != 0:

            cmdstring="python "+connect_py+" "+username+" "+password+" "+outdir+" "+vm_ip

            if systype == "linux" or systype == "linux2":
                p = os.popen("python2.7 %s %s %s %s %s" % (connect_py,username,password,outdir,vm_ip),"w",1)
            else:

                if len(username) != 0:
                    p = os.popen("python %s %s %s %s %s" % (connect_py,username,password,outdir,vm_ip),"w",1)
                else:
                    p = os.popen("python %s %s %s %s" % (connect_py,password,outdir,vm_ip),"w",1)

            time.sleep(8)
        img_org = Image.open(self.run_parameters['psnr_picture'])
        img_cap = Image.open(vm_ip+'.jpg')
        org_array = numpy.array(img_org)
        cap_array = numpy.array(img_cap)

        psnr = self.psnr(org_array, cap_array)
        print(psnr)
        return psnr


    def load_ip(self,ipfile):
        for i in open(ipfile):
            ip=i.rstrip('\n')
            self.iplist.append(ip)
            self.listWidget.add_vm_image(ip)

    def generate_rightclick_menu(self, pos):
        print(pos)

        menu = QMenu()
        item1 = menu.addAction("PSNRFileUpload")
        item2 = menu.addAction("PSNR")
        item3 = menu.addAction("Execute XML")

        screenPos = self.listWidget.mapToGlobal(pos)

        action = menu.exec(screenPos)
        if action == item2:
            selectedItems = self.listWidget.selectedItems()
            if selectedItems:
                item = self.listWidget.currentItem()
                ipname = item.text()
                psnr = self.do_psnr(ipname)
                org_tooltip = item.toolTip()
                item.setToolTip(org_tooltip + "PSNR: " + str(psnr))
        elif action == item1:
            selectedItems = self.listWidget.selectedItems()
            if selectedItems:
                item = self.listWidget.currentItem()
                ipname = item.text()
                self.do_file_transfer(ipname)
        elif action == item3:
            selectedItems = self.listWidget.selectedItems()
            if selectedItems:
                item = self.listWidget.currentItem()
                ipname = item.text()
                self.do_exec_xml(ipname)

    def singleclicktimeout(self):
        if self.doubleclicked == False:

#            self.qmp.connect('172.16.10.1',53002)

            selectedItems = self.listWidget.selectedItems()
            if selectedItems:
                item = self.listWidget.currentItem()
                ipname = item.text() + ".jpg"
                if os.path.exists(ipname) == False:
                    ipname = self.run_parameters['default_icon_name']
                pic_path = "border-image:url("+ ipname + ");color:rgb(0,255,0)"
                self.showLabel.setStyleSheet(pic_path)
                self.showLabel.setText(item.toolTip())
                width,height = Image.open(ipname).size
                self.showLabel.resize(width,height)

        else:
            self.doubleclicked = False

    def click(self):
        self.singleclicktimer.start(300)


    def dbclick(self):
        print("Double Clicked !!!")
        self.doubleclicked = True
        selectedItems = self.listWidget.selectedItems()
        if selectedItems:
            item = self.listWidget.currentItem()
            systype=sys.platform
            connect_type = self.run_parameters['remote_desktop_default_type']
            username="-u " + self.run_parameters['username']
            password="-p " + self.run_parameters['password']
            if systype == "linux" or systype == "linux2":
                outdir = "-o " + os.getcwd() + "/"
                if connect_type == "RDP":
                    connect_py = self.run_parameters['linux_remotedesktop_path']
                else:
                    connect_py = self.run_parameters['linux_vnc_path']
                    username=""
                    password="-p " + self.run_parameters['vnc_password']
            else:
                outdir = "-o " + os.getcwd() + "\\"
                if connect_type == "RDP":
                    connect_py = self.run_parameters['win_remotedesktop_path']
                else:
                    connect_py = self.run_parameters['win_vnc_path']
                    username=""
                    password="-p " + self.run_parameters['vnc_password']
            if systype == "linux" or systype == "linux2":
                p = os.popen("python2.7 %s %s %s %s" % (connect_py,username,password,item.text()),"w",1)
            else:

                if len(username) != 0:
                    p = os.popen("python %s %s %s %s" % (connect_py,username,password,item.text()),"w",1)
                else:
                    p = os.popen("python %s %s %s" % (connect_py,password,item.text()),"w",1)
            self.vmupdatetimer.stop()

app = QtWidgets.QApplication(sys.argv)
app.setStyleSheet('.QLabel { font-size: 16pt;}' )
mt = VMPanel()
mt.setWindowTitle("VM Monitor")

if 'ip_listfile_name' in mt.run_parameters:
    mt.load_ip(mt.run_parameters['ip_listfile_name'])
    mt.show()
    app.exec_()
else:
    print("vmmonitor.config is not correct, exit")