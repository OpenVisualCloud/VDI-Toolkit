
# -*- coding: utf-8 -*-
import win32api
import win32con
import win32clipboard as c
import subprocess
import time
import os
import pyautogui
import psutil

def open_vscode(vscodepath, waittime):
    subprocess.Popen(vscodepath)
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
    pyautogui.PAUSE = 0.5
    # pyautogui.FAILSAFE = True
    pyautogui.typewrite(r'C:\Users\Administrator\Downloads\hello.c')
    time.sleep(1)
    pyautogui.press('enter') #按下并释放enter
    time.sleep(1)

    # 按下Ctrl键
    #pyautogui.keyDown('ctrl')

    # 按下a键，拷贝
    #pyautogui.press('a')

    # 按下c键，复制
    #pyautogui.press('c')

    # 松开Ctrl键
    #pyautogui.keyUp('ctrl')
    #time.sleep(5)

  


def vscode_init():
    alert = 'vscode has open successfully.'

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

def vscode_main(mainstr):
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

def close_vscode():
    win32api.keybd_event(18, 0, 0, 0)
    win32api.keybd_event(115, 0, 0, 0)
    win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)
    
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

    win32api.keybd_event(78, 0, 0, 0)    # n
    win32api.keybd_event(78, 0,win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(79, 0, 0, 0)    # o
    win32api.keybd_event(79, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(84, 0, 0, 0)    # t
    win32api.keybd_event(84, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(69, 0, 0, 0)    # e
    win32api.keybd_event(69, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)    

    win32api.keybd_event(80, 0, 0, 0)    # p
    win32api.keybd_event(80, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(65, 0, 0, 0)    # a
    win32api.keybd_event(65, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(68, 0, 0, 0)    # d
    win32api.keybd_event(68, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1) 

    win32api.keybd_event(110, 0, 0, 0)    # .
    win32api.keybd_event(110, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)    

    win32api.keybd_event(84, 0, 0, 0)    # t
    win32api.keybd_event(84, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(88, 0, 0, 0)    # x
    win32api.keybd_event(88, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1)

    win32api.keybd_event(84, 0, 0, 0)    # t
    win32api.keybd_event(84, 0, win32con.KEYEVENTF_KEYUP, 0)
    #time.sleep(1) 

    win32api.keybd_event(13, 0, 0, 0)    # enter
    win32api.keybd_event(13, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)

    # win32api.keybd_event(9, 0, 0, 0)    # tab 
    # win32api.keybd_event(9, 0,win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)

    # win32api.keybd_event(9, 0, 0, 0)    # tab
    # win32api.keybd_event(9, 0, win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)

    # win32api.keybd_event(13, 0, 0, 0)    # enter
    # win32api.keybd_event(13, 0, win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)

    # win32api.keybd_event(40, 0, 0, 0)    # Down Arrow
    # win32api.keybd_event(9, 0, win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)    

    # win32api.keybd_event(40, 0, 0, 0)    # Down Arrow
    # win32api.keybd_event(9, 0, win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)  

    # win32api.keybd_event(13, 0, 0, 0)    # enter
    # win32api.keybd_event(13, 0, win32con.KEYEVENTF_KEYUP, 0)
    # time.sleep(1)


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
    win32api.keybd_event(18, 0, 0, 0)
    win32api.keybd_event(115, 0, 0, 0)
    win32api.keybd_event(115, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(18, 0, win32con.KEYEVENTF_KEYUP, 0)

def edge_play_music():
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
    vscodepath = r'C:\Program Files\Microsoft VS Code\Code.EXE'
    notepadpath = r'C:\WINDOWS\system32\notepad.exe'
    edgepath = r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe'
    chromepath = r'C:\Program Files\Google\Chrome\Application\chrome.exe'

    mainstr = "Hello, World!"
    mainstr2 = "Life is short, you need Python!"

    open_edge(edgepath,waittime=5)
    if if_process_is_running_by_exename(exename='msedge.exe'):
        edge_init()
        edge_play_music()
        start_time = time.time()
        end_time = time.time()
        last_time = 300
        while end_time - start_time < last_time:
            end_time = time.time()
        close_edge()    
    else:
        print('Failed')



    open_vscode(vscodepath,waittime=5)
    if os.path.isfile(r'C:\Users\Administrator\Downloads\hello.c'):
        vscode_init()
        start_time = time.time()
        end_time = time.time()
        last_time = 300
        while end_time - start_time < last_time:
            vscode_main(mainstr)
            end_time = time.time()
        close_vscode()    
    else:
        print('Failed')

        





