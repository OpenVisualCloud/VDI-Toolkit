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
    exit
fi

card_id=$1
vf_count=$2
lmem=$3

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


echo 0 > /sys/class/drm/card"$card_id"/device/sriov_drivers_autoprobe
modprobe vfio_pci
echo 1 > /sys/class/drm/card"$card_id"/device/sriov_drivers_autoprobe
echo 8086 56c0 > /sys/bus/pci/drivers/vfio-pci/new_id

# unbind vfio driver
bus_id=$(udevadm info -q property /dev/dri/card"$card_id" |grep ID_PATH= |cut -d ':' -f 2)
# VFID=$(lspci |grep -ie display |sed '1d' |cut -d ' ' -f 1)
PCI_BDF=$(lspci |grep -ie display |grep -v 00.0 |grep "$bus_id" |cut -d ' ' -f 1)
for i in ${PCI_BDF}
do 
  #echo 0000:$i > /sys/bus/pci/drivers/pcibak/unbind
  echo 0000:"$i" > /sys/bus/pci/drivers/intel_vsec/unbind
done

echo 8086 56c0 > /sys/bus/pci/drivers/vfio-pci/new_id
echo "$vf_count VFs created."
ls /dev/vfio/

