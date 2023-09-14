These scripts are used to generate VDD in the VM. they shall be run with powershell.

1-ManualInstall.bat is used to manually install driver, enable an 2k virtual display, then auto reboot Windows. It is required to unzip the graphics driver package before running this script.
2-SilentInstall.bat is used to silient install driver, enable an 2k virtual display, then auto reboot Windows. It is required to unzip the graphics driver package before running this script.

3- For Flex 140 (device id is 8086 56c1 in lspci & device manager)
use Windows Powershell to run following commands:
install-Flex140.ps1 -enable 2k # enable virtual display and 2k mode
install.Flex140.ps1 -enable 4k # enable virtual display and 4k mode
install.Flex140.ps1 -disable     # disable virtual display

4- For Flex 170 (device id is 8086 56c0 in lspci & device manager)
use Windows Powershell to run following commands:
install-Flex170.ps1 -enable 2k # enable virtual display and 2k mode
install.Flex170.ps1 -enable 4k # enable virtual display and 4k mode
install.Flex170.ps1 -disable     # disable virtual display

After running the script, it is required to reboot VM to let the script take function.