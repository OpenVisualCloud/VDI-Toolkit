#!/usr/bin/env python
# -*- coding:utf-8 -*-

import os
import random
import time
import sys
import string
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
        "TAB":Keys.TAB,
        "CTRL":Keys.CONTROL,
        "CTRL+N": Keys.CONTROL+"N",
        "CTRL+Q": Keys.CONTROL+"Q",
        "CTRL+O": Keys.CONTROL+"O",
        "ALT+F4": Keys.ALT+Keys.F4,
        "ALT+E": Keys.ALT+"E",
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
        print "title %s" % title
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
                print"Remote find window name to match %s" % name
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
        print "Can not find %s" % app_path
        return False
    return app_driver

def setup_app(path):
        #set up appium
        global splashscreen
        global app_name
        global app_url
        global driver
        global app_driver
        global match_handle
        global root_driver
        desired_caps = {}
        if splashscreen == 1:
            print("Launch app with splash screen ")
            desired_caps = {"platformName": "Windows", "deviceName": "WindowsPC", "app": "Root"}
            driver = webdriver.Remote(
                command_executor=app_url,
                desired_capabilities= desired_caps)
            app_driver = launch_app(path)
            #time.sleep(8)
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
            desired_caps["app"] = path
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
    global root_driver
    print "xml : %s" % dirpath
    DOMTree = minidom.parse(dirpath)
    collection = DOMTree.documentElement

    try:
        splash = collection.getElementsByTagName('app_splashscreen')[0]
    except:
        print("No splash!")
        splashscreen = 0
    else:
        splashscreen = string.atoi(splash.childNodes[0].data)

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
        print "app_url : %s" % app_url

    try:
        url = collection.getElementsByTagName('app_class')[0]
    except:
        print("No app_class!")
    else:
        app_class = url.childNodes[0].data
        print "app_class : %s" % app_class

    if collection.hasAttribute("app_path"):
        setup_app(collection.getAttribute("app_path"))
    else:
        print("No app path defined,quit")
    actions = collection.getElementsByTagName("action")
    for action in actions:
        type = action.getElementsByTagName('type')[0]
        if type.childNodes[0].data == "click":
            clickname = action.getElementsByTagName('name')[0]
            try:
                driver.find_element_by_name(clickname.childNodes[0].data).click()
            except:
                driver.find_element_by_id(clickname.childNodes[0].data).click()
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
                randomcount = string.atoi(randomdata.childNodes[0].data)
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
            keycount = string.atoi(count.childNodes[0].data)
            str = get_send_key(editdata.childNodes[0].data)
            for i in range(keycount):
                editBox.send_keys(str)
        elif type.childNodes[0].data == "delay":
            print("Just delay!")
        elif type.childNodes[0].data == "xpath":
            print("Xpath")
        else:
            print("error type!")
        delay = action.getElementsByTagName('delay')[0]
        sleeptime = string.atof(delay.childNodes[0].data)
        time.sleep(sleeptime)

def findAllFile(base):
    for root, ds, fs in os.walk(base):
        for f in fs:
            if f.endswith('.xml'):
               fullname = os.path.join(root, f)
               yield fullname

def main():
    global app_url 
    if len(sys.argv) > 1:
        app_url="http://"+sys.argv[1]+":4723"
        print app_url
    while 1:
        for xmlfile in findAllFile('.'):
            do_test(xmlfile)

if __name__ == '__main__':
    main()

