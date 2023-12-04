#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import random
import time
import sys
import string
import sqlite3
import threading
import socket
if sys.platform == "win32":
    import win32api  # pip install pywin32
    import win32gui
import xml.dom.minidom as minidom
from xml.dom.minidom import parse
from appium import webdriver
from selenium.webdriver.common.keys import Keys

splashscreen = 0
app_name = ""
app_class = None
app_url = "http://127.0.0.1:4723"
root_dir = None
taskmgr = 0
filetransfer_port = 4724

def generate_random_str(len):
    str = ""
    for i in range(len):
        str += random.choice(string.ascii_letters + string.digits + string.punctuation + string.whitespace)
    return str

def get_send_key(key_value):
    str = ""
    funckey_dict = {
        "F1":Keys.F1,
        "F2":Keys.F2,
        "F3":Keys.F3,
        "F4":Keys.F4,
        "F5":Keys.F5,
        "F6":Keys.F6,
        "F7":Keys.F7,
        "F8":Keys.F8,
        "F9":Keys.F9,
        "F10":Keys.F10,
        "F11":Keys.F11,
        "F12":Keys.F12,
        "DELETE":Keys.DELETE,
        "SPACE":Keys.SPACE,
        "BACKSPACE":Keys.BACKSPACE,
        "RETURN":Keys.RETURN,
        "ESC":Keys.ESCAPE,
        "TAB":Keys.TAB,
        "CTRL":Keys.CONTROL,
        "CTRL+N": Keys.CONTROL+"N",
        "CTRL+Q": Keys.CONTROL+"Q",
        "CTRL+O": Keys.CONTROL+"O",
        "ALT+F4": Keys.ALT+Keys.F4,
        "ALT+E": Keys.ALT+"E",
        "ALT+D": Keys.ALT+"D",
    }
    if len(key_value) > 1:
        str =  funckey_dict.get(key_value,"")
    elif len(key_value) == 1:
        str = key_value
    return str

match_handle = None
def check_name(hwnd,name):
    global match_handle
    if sys.platform == "win32":
        title = win32gui.GetWindowText(hwnd)
        print("title %s" % title)
        if title.find(name) > -1:
            match_handle = hwnd

def find_element_by_name_exist(driver, name):
    global match_handle
    global app_class
    try:
        element = driver.find_element_by_name(name)
    except Exception as e:
        try:
            element = driver.find_element_by_class_name(name)
        except Exception as e:
            print("App name not match")
            print(driver.title)
            if app_url == "http://127.0.0.1:4723":         #local test, use win32gui
                if sys.platform == "win32":
                    win32gui.EnumWindows(check_name,name)
            else:                                          # remote, can not use win32gui
                print("Remote find window name to match %s" % name)
                elements = driver.find_elements_by_xpath("//*")
                for element in elements:
                    title = element.get_attribute("Name")
                    if title != None and title.find(name) > -1:
                        if app_class != None:
                            if element.get_attribute("ClassName") == app_class:
                                match_handle = element.get_attribute("NativeWindowHandle")
                                break
                        else:
                            match_handle = element.get_attribute("NativeWindowHandle")
                            break
            return None
    return element

def launch_app(app_path):
    app_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": app_path}
    try:
        app_driver = webdriver.Remote(command_executor=app_url, desired_capabilities=app_caps)
    except Exception as e:
        print("Can not find %s" % app_path)
        return False
    return app_driver

def run_dos_cmd(cmd):
    app_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": "cmd.exe","appArguments": "/c "+cmd}
    print(app_caps)
    try:
        driver = webdriver.Remote(command_executor=app_url, desired_capabilities=app_caps)
    except Exception as e:
        print("Can not find execute command %s error" % cmd)
        return False

def get_ip_byaddress_type(addrtype):
    target_ip = ''
    for ip in socket.gethostbyname_ex(socket.gethostname())[2]:
        if ip.startswith(addrtype):
            target_ip = ip
    return target_ip

def download_thread(ip,src_name,dest_name):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((ip,filetransfer_port))
    server.listen(1)
    (connection,addr) = server.accept()
    print('Got connection from', addr)
    file = open(dest_name,"wb")
    connection.settimeout(3)
    while True:
        try:
            buffer = connection.recv(8192)
            if buffer:
                if len(buffer) > 0:
#                    print("write %d bytes" % len(buffer))
                    file.write(buffer)
        except socket.timeout:
            print("Timeout,disconnecting...")
            break

    connection.close()
    file.close()
    print("Download complete.")


def transfer_file(ip,src_name,dest_name,is_upload):
    if is_upload == True:
        app_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": "cmd.exe",
                    "appArguments": "/c "+ "c:\\test\\nc64.exe -l -p 4724 > " + dest_name}
        print(app_caps)
        try:
            driver = webdriver.Remote(command_executor=app_url, desired_capabilities=app_caps)
        except Exception as e:
            print("Execute command %s error" % (app_caps['appArguments']))
            print(e)
            return False
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((ip,filetransfer_port))
        file = open(src_name,"rb")
        filebuffer = file.read()
        client.sendall(filebuffer)
        client.close()
        print("Send complete!!")
    else:
        strlist = ip.split('.')
        host_ip = get_ip_byaddress_type(strlist[0]+'.'+strlist[1])
        print(host_ip)
        revc_thread = threading.Thread(target=download_thread,args=(host_ip,src_name,dest_name))
        revc_thread.start()
        app_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": "cmd.exe",
                    "appWorkingDir": "c:\\test",
                    "appArguments": "/c "+ "c:\\test\\nc64.exe " + host_ip + " 4724 < " + src_name}
        print(app_caps)
        try:
            driver = webdriver.Remote(command_executor=app_url, desired_capabilities=app_caps)
        except Exception as e:
            print("Can not find execute command %s error" % (app_caps['appArguments']))
        revc_thread.join()


def setup_app(path):
        #set up appium
        global splashscreen
        global app_name
        global app_url
        global app_param
        global driver
        global app_driver
        global match_handle
        global root_driver
        desired_caps = {}
        if splashscreen == 1:
            print("Launch app with splash screen ")
            desired_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": "Root","appArguments": app_param}
            driver = webdriver.Remote(
                command_executor=app_url,
                desired_capabilities= desired_caps)
            app_driver = launch_app(path)
            app = find_element_by_name_exist(driver, app_name)
            if app != None:
                app_handle = app.get_attribute("NativeWindowHandle")
            else:
                app_handle = match_handle
            if app_handle != 0:
                app_caps = {"appTopLevelWindow": str(hex(int(app_handle)))}
                app_driver = webdriver.Remote(command_executor=app_url,
                                                          desired_capabilities=app_caps)
                if app_driver:
                    root_driver = driver
                    driver = app_driver
                else:
                    print("Can not get App driver!")
            else:
                print("Can not get App handle!")
        else:
            desired_caps = {"app": path, "appArguments": app_param}
            print(path)
            driver = webdriver.Remote(
                command_executor=app_url,
                desired_capabilities= desired_caps)
            app_driver = driver
            time.sleep(2)

def do_test(dirpath):
    global splashscreen
    global driver
    global app_name
    global app_url
    global app_class
    global app_param
    global root_driver
    global db_cur
    global vm_ip
    global taskmgr
    sql_text_2 = "INSERT INTO vm_status VALUES(\'" +vm_ip+"\'"
    get_value= {}
    print("xml : %s" % dirpath)
    dom_tree = minidom.parse(dirpath)
    collection = dom_tree.documentElement

    try:
        splash = collection.getElementsByTagName('app_splashscreen')[0]
    except:
        print("No splash!")
        splashscreen = 0
    else:
        splashscreen = int(splash.childNodes[0].data)

    try:
        app = collection.getElementsByTagName('app_name')[0]
    except:
        print("No app_name!")
        app_name = None
    else:
        app_name = app.childNodes[0].data

    try:
        url = collection.getElementsByTagName('app_url')[0]
    except:
        print("No app_url!")
    else:
        app_url = url.childNodes[0].data
        print("app_url : %s" % app_url)

    try:
        url = collection.getElementsByTagName('app_class')[0]
    except:
        print("No app_class!")
    else:
        app_class = url.childNodes[0].data
        print("app_class : %s" % app_class)

    try:
        app_parameter = collection.getElementsByTagName('app_parameter')[0]
    except:
        print("No parameter!")
        app_param = ''
    else:
        app_param = app_parameter.childNodes[0].data

    if collection.hasAttribute("app_path"):
        setup_app(collection.getAttribute("app_path"))
    else:
        print("No app path defined")

    actions = collection.getElementsByTagName("action")
    for action in actions:
        if action.hasAttribute("title"):
            action_title = action.getAttribute("title")
        type = action.getElementsByTagName('type')[0]
        if type.childNodes[0].data == "click":
            clickname = action.getElementsByTagName('name')[0]
            try:
                driver.find_element_by_name(clickname.childNodes[0].data).click()
            except:
                try:
                    driver.find_element_by_id(clickname.childNodes[0].data).click()
                except:
                    try:
                        driver.find_element_by_accessibility_id(clickname.childNodes[0].data).click()
                    except:
                        print("No element found")
        elif type.childNodes[0].data == "edit":
            editname = action.getElementsByTagName('name')[0]
            editBox = driver.find_element_by_class_name(editname.childNodes[0].data)
            try:
                randomdata = action.getElementsByTagName('random')[0]
            except:
                print("No random data")
                editdata = action.getElementsByTagName('string')[0]
                send_str = editdata.childNodes[0].data
            else:
                randomcount = int(randomdata.childNodes[0].data)
                send_str = generate_random_str(randomcount)
            editBox.send_keys(send_str)
        elif type.childNodes[0].data == "key":
            editname = action.getElementsByTagName('name')[0]
            try:
                editBox = driver.find_element_by_class_name(editname.childNodes[0].data)
            except:
                try:
                    editBox = driver.find_element_by_name(editname.childNodes[0].data)
                except:
                    try:
                        editBox = root_driver.find_element_by_name(editname.childNodes[0].data)
                    except:
                        print("Get name failed!")
            editdata = action.getElementsByTagName('keys')[0]
            count = action.getElementsByTagName('count')[0]
            keycount = int(count.childNodes[0].data)
            str = get_send_key(editdata.childNodes[0].data)
            for i in range(keycount):
                if editBox != None:
                    editBox.send_keys(str)
        elif type.childNodes[0].data == "delay":
            print("Just delay!")
        elif type.childNodes[0].data == "xpath":
            print("Xpath")
        elif type.childNodes[0].data == "get":
            getname = action.getElementsByTagName('name')[0]
            ctl_name = None
            try:
                ctl_name = driver.find_element_by_name(getname.childNodes[0].data)
            except:
                try:
                    ctl_name = driver.find_element_by_id(getname.childNodes[0].data)
                except:
                    try:
                        ctl_name = driver.find_element_by_accessibility_id(getname.childNodes[0].data)
                    except:
                        print("Element by access id can not find")

            if( ctl_name != None ):
                str = ctl_name.get_attribute("Name")
                sql_text_2 = sql_text_2 + ",\'" + str + "\'"
                get_value[action_title] = str
                print(action_title+" ")
                print(str)
        elif type.childNodes[0].data == "exe":
            exename = action.getElementsByTagName('name')[0]
            run_dos_cmd(exename.childNodes[0].data)
        elif type.childNodes[0].data == "upload":
            src_name = action.getElementsByTagName('src_name')[0]
            dest_name = action.getElementsByTagName('dest_name')[0]
            transfer_file(vm_ip,src_name.childNodes[0].data,dest_name.childNodes[0].data,True)
        elif type.childNodes[0].data == "download":
            src_name = action.getElementsByTagName('src_name')[0]
            dest_name = action.getElementsByTagName('dest_name')[0]
            transfer_file(vm_ip,src_name.childNodes[0].data,dest_name.childNodes[0].data,False)
        else:
            print("error type!")
        delay = action.getElementsByTagName('delay')[0]
        sleeptime = float(delay.childNodes[0].data)
        time.sleep(sleeptime)
    if(taskmgr):
        insert_db_data(sql_text_2)

def find_all_file(base):
    for root, ds, fs in os.walk(base):
        for f in fs:
            if f.endswith('.xml'):
               fullname = os.path.join(root, f)
               yield fullname

def insert_db_data(sql_text):
    sql_text = sql_text + ")"
    print(sql_text)
    try:
        db_cur.execute(sql_text)
    except:
        print("%s error" % sql_text)
    else:
        db_conn.commit()
    db_conn.close()

def init_db(dbname,xmlname):                 # save status data to sqlite3 database
    global db_cur
    global db_conn
    title_list = []
    dom_tree = minidom.parse(xmlname)
    collection = dom_tree.documentElement
    actions = collection.getElementsByTagName("action")
    for action in actions:
        if action.hasAttribute("title"):
            action_title = action.getAttribute("title")
        type = action.getElementsByTagName('type')[0]
        if type.childNodes[0].data == "get":
            if action.hasAttribute("title"):
                action_title = action.getAttribute("title")
                title_list.append(action_title)

    db_conn = sqlite3.connect(dbname)
    db_cur = db_conn.cursor()
    sql_text_1 = "CREATE TABLE IF NOT EXISTS vm_status(`ip` TEXT"
    for title in title_list:
        sql_text_1 += ",`"+ title +"` TEXT"
    sql_text_1 += ");"
    print(sql_text_1)
    db_cur.execute(sql_text_1)

def set_app_url(url):
    global app_url
    app_url = "http://"+url+":4723"
    print(app_url)

def main():
    global app_url 
    global vm_ip
    global taskmgr
    if len(sys.argv) > 1:
        vm_ip = sys.argv[1]
        app_url="http://"+sys.argv[1]+":4723"
        if len(sys.argv) > 2:
            xmlname = sys.argv[2]
            if xmlname == "taskmgr.rox":
                taskmgr = True
    if taskmgr == True:
        init_db("test.db",xmlname)
        do_test(xmlname)
        return
    if xmlname:
        do_test(xmlname)
        return
    while 1:
        for xmlfile in find_all_file('.'):
            do_test(xmlfile)

if __name__ == '__main__':
    main()

