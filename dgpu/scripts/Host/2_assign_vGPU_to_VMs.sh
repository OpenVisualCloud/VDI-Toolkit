#!/bin/bash 

id=1; for gpu in $(lspci |grep -ie display |grep -v 00.0 | cut -d ' ' -f 1)
do
  virt-xml win2k19-"$id" --remove-device --hostdev all
  virt-xml win2k19-"$id" --add-device --hostdev "$gpu",driver_name=vfio
  id=$((id+1))
done

