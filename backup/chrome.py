
# -*- coding: utf-8 -*-
import win32api
import win32con
import win32clipboard as c
import subprocess
import time
import os
import pyautogui
import psutil

def open_chrome(chromepath, waittime):
    subprocess.Popen(chromepath)
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

  


def chrome_init():
    alert = 'chrome has open successfully.'
    time.sleep(1)
    pyautogui.hotkey('alt', 'space')
    pyautogui.press('x')
    time.sleep(1)


def close_chrome():
    win32api.keybd_event(18, 0, 0, 0)
    win32api.keybd_event(115, 0, 0, 0)
    win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)

def chrome_play_music():
    url1 = r'http://www.baidu.com/'
    url2 = r'https://music.163.com/#/discover/toplist'
    
    # 每个动作间隔0.5秒钟
    pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(url1)
    pyautogui.press('enter') 
    time.sleep(5)
    pyautogui.typewrite(r'netease cloud music')
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(5)

    pyautogui.hotkey('ctrl', 't')
    time.sleep(1)
    pyautogui.typewrite(url2)
    time.sleep(1)
    pyautogui.press('enter')
    time.sleep(15)
    pyautogui.click(x=1284, y=448)
    time.sleep(5)


def chrome_play_video():
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
    # pyautogui.click(x=1284, y=448)
    time.sleep(5)

def if_process_is_running_by_exename(exename):
    for proc in psutil.process_iter(['pid', 'name']):
        # This will check if there exists any process running with executable name
        if proc.info['name'] == exename:
            return True
    return False

if __name__ == "__main__":
    chromepath = r'C:\Program Files\Google\Chrome\Application\chrome.exe'
    mainstr = "Hello, World!"
    exename = 'chrome.exe'
    open_chrome(chromepath,waittime=5)

    if if_process_is_running_by_exename(exename):
        chrome_init()
        chrome_play_video()
        start_time = time.time()
        end_time = time.time()
        last_time = 300
        while end_time - start_time < last_time:
            end_time = time.time()
        close_chrome()    
    else:
        print('Failed')

        







