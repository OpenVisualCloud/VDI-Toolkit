#!/usr/bin/env python
# -*- coding: utf-8 -*-

import win32api
import win32con
import win32clipboard as c
import subprocess
import time
import os
import pyautogui

def open_word(wordpath, waittime):
    subprocess.Popen(wordpath)
    time.sleep(waittime)

    win32api.keybd_event(17, 0, 0, 0)   # ctrl key
    win32api.keybd_event(78, 0, 0, 0)   # n key 
    win32api.keybd_event(78, 0, win32con.KEYEVENTF_KEYUP, 0)   # release n key
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)   # release ctrl key
    time.sleep(1)

    win32api.keybd_event(17, 0, 0, 0)   # ctrl key
    win32api.keybd_event(83, 0, 0, 0)   # s key 
    win32api.keybd_event(83, 0, win32con.KEYEVENTF_KEYUP, 0)   # release s key
    win32api.keybd_event(17, 0, win32con.KEYEVENTF_KEYUP, 0)   # release ctrl key
    time.sleep(1)


    # 每个动作间隔0.5秒钟
    # pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(r'C:\Users\Administrator\Downloads\Doc1.docx')
    time.sleep(1)
    pyautogui.press('enter') #按下并释放enter
    time.sleep(1)


def word_init():
    alert = 'Word has open successfully.'

    c.OpenClipboard()
    c.EmptyClipboard()
    c.SetClipboardData(win32con.CF_UNICODETEXT,alert)
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

def word_main(mainstr):
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

def close_word():
    win32api.keybd_event(18, 0, 0, 0)
    win32api.keybd_event(115, 0, 0, 0)
    win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)
    

if __name__ == "__main__":
    wordpath = r'C:\Users\Administrator\AppData\Local\Kingsoft\WPS Office\10.1.0.7698\office6\wps.exe'
    mainstr = "Hello, World!"
    open_word(wordpath,waittime=5)
    if os.path.isfile(r'C:\Users\Administrator\Downloads\Doc1.docx'):
        word_init()
        start_time = time.time()
        end_time = time.time()
        last_time = 180
        while end_time - start_time < last_time:
            word_main(mainstr)
            end_time = time.time()
        close_word()    
    else:
        print('Failed')

        





