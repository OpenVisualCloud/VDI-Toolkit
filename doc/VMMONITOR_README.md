# vmmonitor.py
## Description: UI to query the guest status and profiling

## installation
```
python3.7 must be installed first

pip install PyQt5
pip install pillow
pip install psutil
pip install numpy

Copy guest vm_ip.txt to the same dir as vmmonitor and test vmmonitor:

python3.7 vmmonitor.py

```

## Usage:
```
vmmonitor.py
```
- The virtual machine need install nc.exe to let the vmmonitor to upload or download files with virtual machine
- Single click the icon of the virtual machine will show the VM latest snapshot
- Double click the icon of the virtual will launch the RDP or VNC window of VM
- The vmmonitor.config is the configuration file of the vmmonitor, from this file you can config the status query interval(ms unit) and VNC or RDP password
- Also, you can stop status qeury using the file menu and set the upload PSNR pattern picture filename
- .ROX file means Run Once Xml file, this file is same structure with xml file, but this kind of file is not in repeated running xml list, and only run once just for special purpose, such as taskmgr,upload file