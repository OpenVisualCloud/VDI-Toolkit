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

def open_edge(edgepath, waittime):
    subprocess.Popen(edgepath)
    time.sleep(waittime)

    # 按下Ctrl键
    #pyautogui.keyDown('ctrl')

    # 按下a键，拷贝
    #pyautogui.press('a')

    # 按下c键，复制
    #pyautogui.press('c')

    # 松开Ctrl键
    #pyautogui.keyUp('ctrl')
    #time.sleep(5)

def edge_init():
    alert = 'edge has open successfully.'
    time.sleep(1)
    pyautogui.hotkey('alt', 'space')
    pyautogui.press('x')
    time.sleep(1)


def close_edge():
    # win32api.keybd_event(18, 0, 0, 0)
    # win32api.keybd_event(115, 0, 0, 0)
    # win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    # win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)
    
    # os.system('"taskkill /F /IM msedge.exe"')
    pyautogui.hotkey('alt', 'f4')

def edge_play_music():
    url1 = r'http://www.baidu.com/'
    url2 = r'https://music.163.com/#/discover/toplist'
    url3 = r'https://github.com/trending'
    url4 = r'http://news.baidu.com/'
    url5 = r'https://www.taobao.com/'
    url6 = r'https://www.google.com/'
    url7 = r'https://www.bilibili.com/medialist/play/697166795?from=space&business=space&sort_field=pubtime&spm_id_from=333.999.0.0'

    
    # 每个动作间隔0.5秒钟
    # pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(url1)
    pyautogui.press('enter') 
    time.sleep(3)
    pyautogui.typewrite(r'netease cloud music')
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(3)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url2)
    time.sleep(1)
    pyautogui.press('enter')
    # time.sleep(15)
    # pyautogui.click(x=1284, y=448)
    time.sleep(3)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url6)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(5)
    pyautogui.typewrite(r'github trending')
    time.sleep(2)
    pyautogui.press('enter')
    

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url3)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(3)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url5)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(3)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url7)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(3)

def edge_play_video():
    url3 = r'https://www.google.com/'
    url4 = r'https://www.bilibili.com/medialist/play/697166795?from=space&business=space&sort_field=pubtime&spm_id_from=333.999.0.0'

    # 每个动作间隔0.5秒钟
    pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(url3)
    pyautogui.press('enter') 
    time.sleep(5)
    pyautogui.typewrite(r'bilibili')
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(5)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url4)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(15)
    pyautogui.click(x=1284, y=448)
    time.sleep(5)

def if_process_is_running_by_exename(exename):
    for proc in psutil.process_iter(['pid', 'name']):
        # This will check if there exists any process running with executable name
        if proc.info['name'] == exename:
            return True
    return False

if __name__ == "__main__":
    edgepath = r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe'
    mainstr = "Hello, World!"
    exename = 'msedge.exe'
    open_edge(edgepath,waittime=3)

    if if_process_is_running_by_exename(exename):
        edge_init()
        edge_play_music()
        start_time = time.time()
        end_time = time.time()
        last_time = 180
        while end_time - start_time < last_time:
            end_time = time.time()
        close_edge()    
    else:
        print('Failed')

        







