# How to get MRDA encode/decode e2e frame latency

## 1. Turn on trace macro definition compiler flags -DENABLE_TRACE=ON in
- MediaResourceDirectAccess/Scripts/Linux/install_host.sh
- MediaResourceDirectAccess/Scripts/Windows/lib/WinBuild.bat
- Examples/SampleEncodeApp/scripts/WinBuild.bat

## 2. Run the MRDA encode/decode sample app
- Synchronize the host and guest time
1. Host:
```
ntpdate corp.intel.com
```
2. Guest:
```
"Go to 'Control panel' > 'Clock and Region' > 'Date and Time' > 'Internet Time', check 'Synchronize with an Internet time server', and click 'Update Now' to sync with "corp.intel.com" time."
```
- Host command:
```
sudo ./HostService -addr 127.0.0.1:50051 &> host.txt
```
- Guest command:(MRDASampleDecodeApp for example)
```
./MRDASampleDecodeApp.exe --hostSessionAddr 127.0.0.1:50051 -i input.h265 -o output.raw --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --decodeType ffmpeg > guest.txt
```
## 3. Get the trace data
- Put guest.txt to Host side and do file convert:
```
dos2unix guest.txt
```
- Get the trace data from host.txt and guest.txt
```
Usage: ./mrda_single_decoding_latency_profile.sh host_file guest_file start_frame_num end_frame_num
```
- Get the result:
```
e2e average: xxx ms
```