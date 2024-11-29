# MDSCMRDASample Guide
This sample contains the MDSCMRDASampleApp.exe application, which demonstrates how to utilize the MultiDisplayScreenCapture library to capture the screen of a Windows virtual machine (VM) and leverage the MediaResourceDirectAccess feature to access the host machine's hardware resources for hardware-accelerated video encoding. Finally, it uses FFmpeg to stream the encoded video from the virtual machine.

The dependencies of MDSCMRDASample include ``MultiDisplayScreenCapture.dll``, ``libWinGuest.dll`` and ``ffmpeg-6.1.1-full_build-shared`` which can be found in the compiled multidispscreencap and MediaResourceDirectAccess modules.
To enable rtsp streaming, ``mediamtx_v1.6.0_windows_amd64`` is downloaded to start the rtsp server. And CMakeList.txt and WinBuild.bat scripts can be used to compile the binary.

## Usage:
### 1. IVSHMEM device and Host service
Please refer to the [MediaResourceDirectAccess README.md](..\MediaResourceDirectAccess\README.md) for the setup of IVSHMEM device and Host service.

### 2. Windows Guest Sample App
Besides the params of MRDASampleApp.exe in the [MediaResourceDirectAccess README.md](..\MediaResourceDirectAccess\README.md),
MDSCMRDASampleApp.exe added 3 params for inputType and outputType selection:

[--inputType input_type]	- specifies the input type. option: capture, file.

[--outputType output_type]	- specifies the ouput type. option: stream, file.

[-rtsp rtsp_url]			- specifies output rtsp url.

You can choose screencap or file input, and stream or file output.

If stream output is selected, rtsp server needs to be started in the Windows VM first:
```bash
cd scripts\build\Release\mediamtx_v1.6.0_windows_amd64
.\mediamtx.exe
```
And the IP port "8554" of the Windows virtual machine needs to be forwarded to the host IP port:
```bash
sudo iptables -t nat -I PREROUTING -p tcp --dport 38554 -j DNAT --to ${IP}:8554
```

Then start the MDSCMRDASample:
```bash
cd scripts\build\Release
# 1. Run MDSCMRDASampleApp.exe with capture input and stream output:
.\MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051  --inputType capture --outputType stream --rtsp rtsp://127.0.0.1:8554/screencap --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL
# 2. Run MDSCMRDASampleApp.exe with capture input and file output:
.\MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 --inputType capture --outputType file -o output.hevc --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL
# 3. Run MDSCMRDASampleApp.exe with file input and file output:
.\MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 --inputType file --outputType file -i input.rgba -o output.hevc --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL
# 4. Run MDSCMRDASampleApp.exe with file input and stream output:
.\MDSCMRDASampleApp.exe --hostSessionAddr 127.0.0.1:50051 --inputType file --outputType stream -i input.rgba --rtsp rtsp://127.0.0.1:8554/screencap --memDevSize 1000000000 --bufferNum 100 --bufferSize 10000000 --inDevPath /dev/shm/shm1IN --outDevPath /dev/shm/shm1OUT --inDevSlotNumber 11 --outDevSlotNumber 12 --frameNum 3000 --codecId h265 --gopSize 30 --asyncDepth 4 --targetUsage balanced --rcMode 1  --bitrate 15000 --fps 30 --width 1920 --height 1080 --colorFormat rgb32 --codecProfile hevc:main --maxBFrames 0 --encodeType oneVPL
```

To read from the published rtsp stream, you can use ffplay in your client machine:
```bash
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38554/capture
```

## Compile:
Open Developer PowerShell for VS 2022 and run below commands:
```bash
cd scripts
.\WinBuild.bat
```
And the binary can be found in "scripts\build\Release" folder.

## License
MDSCMRDASample module is licensed under MIT license. Note that MIT license is only the license for Intel VDI Tool Kit itself, independent of its third-party dependencies, which are separately licensed.

One of the dependecies FFmpeg is an open source project licensed under LGPL and GPL. See https://www.ffmpeg.org/legal.html. You are solely responsible for determining if your use of FFmpeg requires any additional licenses. Intel is not responsible for obtaining any such licenses, nor liable for any licensing fees due, in connection with your use of FFmpeg.