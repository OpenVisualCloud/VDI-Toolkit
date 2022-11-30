#!/usr/bin/env python
# -*- coding: utf-8 -*-

import win32api
import win32con
import win32clipboard as c
import subprocess
import time
import os
import pyautogui
import psutil

def open_vlc(vlcpath, waittime):
    subprocess.Popen(vlcpath)
    time.sleep(waittime)


def vlc_init():
    alert = 'vlc has open successfully.'
    time.sleep(1)
    pyautogui.hotkey('alt', 'space')
    pyautogui.press('x')
    time.sleep(1)


def close_vlc():
    # win32api.keybd_event(18, 0, 0, 0)
    # win32api.keybd_event(115, 0, 0, 0)
    # win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    # win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)
    os.system('"taskkill /F /IM vlc.exe"')


def vlc_play_video():

    # 每个动作间隔0.5秒钟
    # pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.hotkey('ctrl', 'o')
    time.sleep(1)
    pyautogui.typewrite(r'C:\Users\Administrator\Desktop\test.mp4')
    time.sleep(1)
    pyautogui.press('enter') 
    time.sleep(1)


def if_process_is_running_by_exename(exename):
    for proc in psutil.process_iter(['pid', 'name']):
        # This will check if there exists any process running with executable name
        if proc.info['name'] == exename:
            return True
    return False

if __name__ == "__main__":
    vlcpath = r'C:\Program Files\VideoLAN\VLC\vlc.exe'
    mainstr = "Hello, World!"
    exename = 'vlc.exe'
    open_vlc(vlcpath,waittime=3)

    if if_process_is_running_by_exename(exename):
        vlc_init()
        vlc_play_video()
        start_time = time.time()
        end_time = time.time()
        last_time = 180
        while end_time - start_time < last_time:
            end_time = time.time()
        close_vlc()    
    else:
        print('Failed')

        







