#!/bin/bash 

if [ $# = 0 ]
then
    echo "Usage: $0 card_id vf_count lmem(GB)"
    echo "e.g.:(card 0)" 
    echo "$0 0 16 1"
    echo "$0 0 8 2"
    echo "$0 0 4 4"
    echo "$0 0 2 8"
    echo "$0 0 1 16"
    echo "e.g.:(card 1)" 
    echo "$0 1 16 1"
    echo "$0 1 8 2"
    echo "$0 1 4 4"
    echo "$0 1 2 8"
    echo "$0 1 1 16"
    echo "Usage2: $0 card_pci_addr vf_count lmem(GB)"
    echo "e.g.:(4d:00.0)"
    echo "$0 4d:00.0 16 1"
    echo "$0 4d:00.0 8 2"
    echo "$0 4d:00.0 4 4"
    echo "$0 4d:00.0 2 8"
    echo "$0 4d:00.0 1 16"
    exit
fi

OS=$(< /etc/os-release grep "PRETTY_NAME" | awk -F '=' '{print $2}')
in_card_id=$1
vf_count=$2
lmem=$3
card_id=$in_card_id

reg_num='^[0-9]{1,2}$'
reg_pci='^[0-9a-f]{2}:[0-9a-f]{2}.[0-9a-f]$'


if [[ "$in_card_id" =~ $reg_num ]];then
    echo "num found"
    card_id=$in_card_id
fi

if [[ "$in_card_id" =~ $reg_pci ]];then
    # shellcheck disable=SC2010
    # shellcheck disable=SC2062
    GpuStr=$(ls -l /sys/class/drm | grep "$in_card_id" | grep -oe card[0-9a-f])
    card_id=${GpuStr[0]:4:1}
fi

echo "card id is: $card_id"

# create VFs with xpu-smi or xpumcli tool
if command -v xpu-smi >/dev/null 2>&1 ; then
    echo "xpu-smi is installed"
    xpu-smi vgpu -d "$card_id" -r -y
    sleep 3
    xpu-smi vgpu -d "$card_id" -c -n "$vf_count" --lmem "$lmem"000
elif command -v xpumcli >/dev/null 2>&1 ; then
    echo "xpumcli is installed"
    xpumcli vgpu -d "$card_id" -r -y
    sleep 3
    xpumcli vgpu -d "$card_id" -c -n "$vf_count" --lmem "$lmem"000
else
    echo "xpu-smi or xpumcli don't installed, please install one of them firstly!"
    exit
fi

unbind_vfio()
{
    echo 0 > /sys/class/drm/card"$card_id"/device/sriov_drivers_autoprobe
    modprobe vfio_pci
    echo 1 > /sys/class/drm/card"$card_id"/device/sriov_drivers_autoprobe

    # unbind vfio driver
    bus_id=$(udevadm info -q property /dev/dri/card"$card_id" |grep ID_PATH= |cut -d ':' -f 2)
    # VFID=$(lspci |grep -ie display |sed '1d' |cut -d ' ' -f 1)
    PCI_BDF=$(lspci |grep -ie display |grep -v 00.0 |grep "$bus_id" |cut -d ' ' -f 1)

    for i in ${PCI_BDF}
    do
        TPREFIX="Kernel driver in use: "
        KMD_INUSE=$(lspci -s $i -vvv | grep "Kernel driver")
        if [ -z "$KMD_INUSE" ]
        then
            echo "no need to unbind kmd for device $i."
        else
            kmd_inuse=$(echo $KMD_INUSE | grep -oP "^$TPREFIX\K.*")
            if [ "$kmd_inuse" = "vfio-pci" ];then
                echo "no need to unbind kmd for device $i."
            else
                echo "unbinding $kmd_inuse for device $i..."
                #echo 0000:$i > /sys/bus/pci/drivers/pcibak/unbind
                echo 0000:"$i" > /sys/bus/pci/drivers/$kmd_inuse/unbind
            fi
        fi
    done

    PCI_ID=$(grep -i PCI_ID /sys/class/drm/card"$card_id"/device/uevent)
    PREFIX="PCI_ID=8086:"
    DEV_ID=${PCI_ID#"$PREFIX"}
    dev_id=${DEV_ID,,}
    echo 8086 $dev_id > /sys/bus/pci/drivers/vfio-pci/new_id
    echo "$vf_count VFs created."
    ls /dev/vfio/
}

if [[ "$OS" =~ .*CentOS.* ]]
then
    unbind_vfio

elif [[ "$OS" =~ .*Rocky.* ]]
then
    echo "Done! Rocky Linux 9 don't need unbind vfio driver."

else
    echo "No support yet."
fi

