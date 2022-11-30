# Windows Workload Emulate Test

This is a sample test project that runs and validates basic UI scenarios on Windows Server 2016. This sample is created as the most basic test project to quickly Use python modules to generate binary/exe file and run in VMs to emulate: operate web browser, office and edit scenario workload ，add 10% VMs with video player workload. 

This guide highlights the following basic instruction to demonstrate how you can setup host and VM guest.

## Requirements

- Server
- Centos 7.x iso package
- Microsoft Windows Server 2016 system iso package
- Test Tools:
  - Visual Studio Code
  - WPS
  - Microsoft Edge
  - Chrome
  - VLC
  - Python v3.8 or higher
  
### Host Setup (Centos7)

- Install Centos7.x
- Disable firewall and selinux
  - `sudo systemctl disable firewall.d`
  - `vi /etc/selinux/config`  modify `SELINUX=disable`

- Install dependencies
  - `yum  -y install libvirt qemu-kvm virt-manager virt-install bridge-utils virt-viewer OVMF.noarch qemu-system-x86.x86_64`
- Build qemu-kvm from source
  - [centos7 update qemu-kvm from source](https://developer.aliyun.com/article/940170)
- Run `virt-manager` opne Virtual Machine Manager GUI to install VM system(Windows Server 2016)
- After install finished, start VMs.
  - `virsh start $vm_name`

### VM Guest Setup (Windows Server 2016)

#### Windows System Setting

- Disable Firewall
- Remove Windows password on login
  - [How to Remove Windows password on login](http://www.cangchou.com/492533.html)
- Enable Remote Desktop
- Unzip and copy the test script and test tool to VM Windows.
- Create a shortcut of run.bat to Windows desktop
- Copy or move the file 'run.bat - Shortcut' to Windows startup boot folder
  - `C:\Users\administrator\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`
- Enable Windows Test Mode

#### Install test tools
Note: Check the test tools default install path as following:
- Test Tools:
  - Visual Studio Code
    - Test script default setting path: `C:\Program Files\Microsoft VS Code\`
  - WPS 2016
    - Test script default install path: `C:\Users\Administrator\AppData\Local\Kingsoft\WPS Office\10.1.0.7698\office6\wps.exe`
  - Microsoft Edge
    - Test script default install path: `C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe`
  - Chrome
    - Test script default install path: `C:\Program Files\Google\Chrome\Application\chrome.exe`
  - VLC
    - Test script default install path: `C:\Program Files\VideoLAN\VLC\vlc.exe`
  - Python v3.8 or higher
    - Add python environment variables
    - Use Python package manager (PIP) to install the following dependencies
      - pywin32 (304)
      - pyautogui (0.9.53)
      - psutil (5.9.2)
      - setuptools (41.2.0)

## Getting Started

- Start VMs from host terminal
  - `virsh start $vm_name`

- The Windows VMs start run the test script automatically do below test while boot into Windows desktop.

  - Open Microsoft Edge, open serverl web pages on new tabs then play video online.
  - Open WPS word, cycle input some text.
  - Open VS Code, cycle input some text.
  - Open VLC player, cycle play 4k video locally.
  - Open Windows notepad, cycle input some text.
