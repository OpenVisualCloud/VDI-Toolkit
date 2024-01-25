#!/bin/bash

# Get the number of VFs
vf_count=$(lspci |grep -ie display |grep -cv 00.0)

# Start Windows VMs
for id in $(seq 1 "$vf_count")
do
  virsh start win2k19-"$id"
done
