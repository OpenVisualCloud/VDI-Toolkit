# Use balloon.sh to get additional vm free memory to host memory pool
## Usage:  balloon.sh monitor_dir polling_interval passive|normal|aggressive
monitor_dir is the VM monitor file directory, normally is the same directory with VM image
polling_interval is the interval to polling the VM's free memory, default value is 2 seconds
passive|normal|aggressive: 3 memory get modes, passive will use VM's 1/4 free memory, normal will use VM's 1/2 memory, aggressive will use VM's 3/4 free memory

For example:
```
./balloon.sh /nvme1 1 aggressive 
```
### This example will use VM's /nvme1 directory and set polling interval to 1 seconds, and use aggressive mode that will get VM's 3/4 memory 