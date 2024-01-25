#!/bin/bash

# Start Windows VMs
for id in {1..32}
do
  virsh shutdown win2k19-"$id"
  sleep 1
done

