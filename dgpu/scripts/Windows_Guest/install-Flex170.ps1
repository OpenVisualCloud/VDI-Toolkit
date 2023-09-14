#Set-ExecutionPolicy -Force -Scope Process -ExecutionPolicy Unrestricted

#----------------------------
# enable: enable virt display, use as -enable "2k/2K/4k/4K"
# disable: disable virt display
#----------------------------

Param(
    [Parameter(Mandatory = $true, ParameterSetName="enable", HelpMessage="use -enable with '2k','2K','4k','4K'")]
    [ValidateSet('2k','2K','4k','4K')]
    [string]$enable,
    
    [Parameter(Mandatory = $true, ParameterSetName="disable", HelpMessage= "use -disable")] [switch]$disable
)

if ($PSBoundParameters.Keys -eq "enable")
{
    Write-Host "Try to enable virt display"
}
elseif ($PSBoundParameters.Keys -eq "disable")
{
    Write-Host "Try to disable virt display"
}
else 
{
    Write-Host "Input error: "
    Write-Host "   [1].Use -enable with '2k' or '4k' to enable virt display"
    Write-Host "   [2].Or use -disable to disable virt display"    
}

if ($enable -eq "2k" -or $enable -eq "2K")
{
    Write-Host "Select 2k mode"
}
elseif ($enable -eq "4k" -or $enable -eq "4K")
{
    Write-Host "Select 4k mode"
}

[String]$driverKey = "HKLM\SYSTEM\CurrentControlSet\Control\Class\"

$devices = Get-CimInstance -ClassName Win32_PnPEntity -Filter "PNPClass = 'DISPLAY' AND Manufacturer = 'Intel Corporation'"

[string]$deviceId
foreach ($device in $devices)
{
    Write-Host "==========================================="
    $deviceId = $device.DeviceID
    Write-Host "DeviceID is : $($deviceId)"

    if (!$deviceId.Contains("PCI\VEN_8086&DEV_56C0"))
    {
        Write-Host "Don't match the DeviceID"
        continue
    }
    $properties = Invoke-CimMethod -MethodName GetDeviceProperties -InputObject $device | Select-Object -ExpandProperty DeviceProperties

    
    foreach ($ele in $properties)
    {
        if ($ele.KeyName -eq "DEVPKEY_Device_Driver")
        {
            $driverKey += $ele.Data
            # Write-Host "driverKey is $($driverKey)"
            if ($disable -eq $true)
            {
                REG ADD $driverKey /v "ForceVirtualDisplay" /t REG_DWORD /d 0x0 /f
            }
            else
            {
                REG ADD $driverKey /v "ForceVirtualDisplay" /t REG_DWORD /d 0x1 /f
                if ($enable -eq "2k" -or $enable -eq "2K")
                {
                    REG ADD $driverKey /f /v "CustomVirtualDisplayEdid" /t REG_BINARY /d 00ffffffffffff00367f010001000000000c0104a50000ff2f0000a057499b2610484f00000001010101010101010101010101010101565e00a0a0a0295030203a0000000000001a000000fd003c00d30019000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000dd
                }
                elseif ($enable -eq "4k" -or $enable -eq "4K")
                {
                    REG ADD $driverKey /f /v "CustomVirtualDisplayEdid" /t REG_BINARY /d 00ffffffffffff00367f010001000000000c0104a50000ff2f0000a057499b2610484f000000010101010101010101010101010101014cd000a0f0703e8030203a0000000000001a000000fd003c00c1003600000000000000000000000000000000000000000000000000000000000000000000000000000000000000000005
                }
            }
        }
    }
}

