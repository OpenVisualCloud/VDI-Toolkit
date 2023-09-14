#!/bin/bash

# Get the number of VFs
vf_count=$(lspci |grep -ie display |grep -v 00.0 | wc -l)

# Start Windows VMs
for id in $(seq 1 $vf_count)
do
  virsh start win2k19-$id
done
