#!/usr/bin/env python
# -*- coding: utf-8 -*-

# import pyautogui
import win32api
import win32con
import win32clipboard as c
import subprocess
import time
import os
import pyautogui

def open_notepad(notepadpath, waittime):
    subprocess.Popen(notepadpath)
    time.sleep(waittime)

    # win32api.keybd_event(13, 0, 0, 0)   # enter
    # win32api.keybd_event(13, 0, win32con.KEYEVENTF_KEYUP, 0)  # release key
    # time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)   # ctrl key
    win32api.keybd_event(83, 0, 0, 0)   # s key 
    win32api.keybd_event(83, 0, win32con.KEYEVENTF_KEYUP, 0)   # release s key
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)   # release ctrl key
    time.sleep(1)

    # 每个动作间隔0.5秒钟
    pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(r'C:\Users\Administrator\Documents\notepad.txt')
    time.sleep(1)
    pyautogui.press('enter') #按下并释放enter
    time.sleep(1)

def notepad_init():
    alert = 'notepad open sucess already.'

    c.OpenClipboard()
    c.EmptyClipboard()
    c.SetClipboardData(win32con.CF_UNICODETEXT,alert)
    c.CloseClipboard()
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)    # ctrl
    win32api.keybd_event(86, 0, 0, 0)    # V
    win32api.keybd_event(86, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    win32api.keybd_event(9, 0, 0, 0)    # tab
    win32api.keybd_event(9, 0,win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)
 
    now_time = str(time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time())))
    c.OpenClipboard()
    c.EmptyClipboard()
    c.SetClipboardData(win32con.CF_UNICODETEXT,now_time)
    c.CloseClipboard()
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)
    win32api.keybd_event(86, 0, 0, 0)
    win32api.keybd_event(86, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    win32api.keybd_event(13, 0, 0, 0)    # enter
    win32api.keybd_event(13, 0,win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

def notepad_main(mainstr):
    c.OpenClipboard()
    c.EmptyClipboard()
    c.SetClipboardData(win32con.CF_UNICODETEXT,mainstr)
    c.CloseClipboard()
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)
    win32api.keybd_event(86, 0, 0, 0)
    win32api.keybd_event(86, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    win32api.keybd_event(9, 0, 0, 0)
    win32api.keybd_event(9, 0,win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    now_time = str(time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time())))
    c.OpenClipboard()
    c.EmptyClipboard()
    c.SetClipboardData(win32con.CF_UNICODETEXT,now_time)
    c.CloseClipboard()
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)
    win32api.keybd_event(86, 0, 0, 0)
    win32api.keybd_event(86, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    win32api.keybd_event(13, 0, 0, 0)
    win32api.keybd_event(13, 0,win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)
    win32api.keybd_event(83, 0, 0, 0)
    win32api.keybd_event(83, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

def close_notepad():
    win32api.keybd_event(18, 0, 0, 0)
    win32api.keybd_event(115, 0, 0, 0)
    win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)
    

if __name__ == "__main__":
    notepadpath = r'C:\WINDOWS\system32\notepad.exe'
    mainstr = "Life is short, you need Python!"
    open_notepad(notepadpath,waittime=3)
    if os.path.isfile(r'C:\Users\Administrator\Documents\notepad.txt'):
        notepad_init()
        start_time = time.time()
        end_time = time.time()
        last_time = 120
        while end_time - start_time < last_time:
            notepad_main(mainstr)
            end_time = time.time()
        close_notepad()    
    else:
        print('Failed')

        





