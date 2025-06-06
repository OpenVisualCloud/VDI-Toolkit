# MDSCMRDASample Guide
This sample contains the MDSCMRDASampleApp.exe application, which demonstrates how to utilize the MultiDisplayScreenCapture library to capture the screen of a Windows virtual machine (VM) and leverage the MediaResourceDirectAccess feature to access the host machine's hardware resources for hardware-accelerated video encoding. Finally, it uses FFmpeg to stream the encoded video from the virtual machine.

The dependencies of MDSCMRDASample include ``MultiDisplayScreenCapture.dll``, ``libWinGuest.dll`` and ``ffmpeg-6.1.1-full_build-shared`` which can be found in the compiled multidispscreencap and MediaResourceDirectAccess modules.
To enable rtsp streaming, ``mediamtx_v1.6.0_windows_amd64`` is downloaded to start the rtsp server. And CMakeLists.txt and WinBuild.bat scripts can be used to compile the binary.

## Usage:
### 1. IVSHMEM device and Host service
Please refer to the [MediaResourceDirectAccess README.md](..\..\MediaResourceDirectAccess\README.md) for the setup of IVSHMEM device and Host service.

### 2. Windows Guest Sample App
Besides the params of MRDASampleApp.exe in the [MediaResourceDirectAccess README.md](..\..\MediaResourceDirectAccess\README.md),
MDSCMRDASampleApp.exe added 3 params for inputType and outputType selection:

`--inputType input_type` - specifies the input type. Options: capture, file.

`--outputType output_type` - specifies the output type. Options: stream, file.

`-rtsp rtsp_url` - specifies the output RTSP URL.

You can choose screencap or file input, and stream or file output.

If stream output is selected, the RTSP server needs to be started in the Windows VM first:
```bash
cd scripts\build\Release\mediamtx_v1.6.0_windows_amd64
.\mediamtx.exe
```
And the IP port "8554" of the Windows virtual machine needs to be forwarded to the host IP port:
```bash
sudo iptables -t nat -I PREROUTING -p tcp --dport 38554 -j DNAT --to ${IP}:8554
```
This sample also has ffmpeg-software and QES encode mode. To use QES encode, IVSHMEM needs to be removed and VGPU needs to be added.

To use multiple displays, Win2k19 with multi-VDD or WIN10 with multi-IDD is needed to set up, and multiple IVSHMEM devices need to be installed for MRDA encode type.
```bash
## Set multiple shmem names with same basic name and add 0,1,2... to the end of the name in xml
## Set serialized slot number for each shmem name in xml
# Display 1:
<shmem name='shm1IN0'>, slot='0x11',
<shmem name='shm1OUT0'>, slot='0x12',
# Display 2:
<shmem name='shm1IN1'>, slot='0x13',
<shmem name='shm1OUT1'>, slot='0x14',

## Set the basic shmem name and the slot number of the first display in the config file
"MRDA-inDevPath": "/dev/shm/shm1IN", "MRDA-inDevSlotNumber": 11,
"MRDA-outDevPath": "/dev/shm/shm1OUT","MRDA-outDevSlotNumber": 12,
```

Edit ``MDSCMRDASample.conf`` for different usage.
```bash
# 1. MRDA params:
## MRDA host session address $host_ip:host_port
"MRDA-host-session-address": "127.0.0.1:50051",
## MRDA total memory size
"MRDA-memDevSize": 1000000000,
## MRDA buffer number
"MRDA-bufferNum": 100,
## MRDA buffer size
"MRDA-bufferSize": 10000000,
## MRDA input memory dev path, multiple displays set same basic DevPath name
"MRDA-inDevPath": "/dev/shm/shm1IN",
## MRDA output memory dev path, multiple displays set same basic DevPath name
"MRDA-outDevPath": "/dev/shm/shm1OUT",
## input memory dev slot number, multiple displays set the slot number of the first display
"MRDA-inDevSlotNumber": 11,
## output memory dev slot number, multiple displays set the slot number of the first display
"MRDA-outDevSlotNumber": 12,

# 2. IO params:
## input type, capture or file
"inputType": "capture",
## output type, stream or file
"outputType": "stream",
## input filename, multiple inputs split with ",", such as "input1.rgba,input2.rgba"
"inputFile": "input.rgba",
## output filename
"outputFile": "output.hevc",
## rtsp server ip
"rtsp-server-ip": "127.0.0.1",
## rtsp server port
"rtsp-server-port": "8554",
## input frame width, capture mode will be modified with captured frame info
"inputWidth": 1920,
## input frame height, capture mode will be modified with captured frame info
"inputHeight": 1080,
## total frame number to capture and encode
"frameNum": 300,


# 3. capture params:
## capture display with fix fps
"capture-fps": 30,
## capture single display("true") or multiple displays("false")
"capture-single-display": false,
## when capture-single-display is "true", select which display to capture, and if the number is larger than the total number of multi-displays, the default display with number 0 will be captured
"capture-single-display-number": 0,
## the path to save "*.rgba" files when "DUMP_RGBA" is defined when building the sample
"capture-dump-path": "capture_dump"

# 2. encode params:
## encode type, "ffmpeg-software", "ffmpeg-MRDA", "vpl-MRDA" or "QES"
"encode-type": "ffmpeg-software",
## input color format, capture mode default "rgb32", file mode support "rgb32", "nv12" and "yuv420p", currently QES encode type not support "yuv420p"
"inputColorFormat": "rgb32",
## output pixel format, "nv12" or "yuv420p"
"outputPixelFormat": "yuv420p",
## codec id, "avc", "hevc", "h264", or "h265"
"encode-codecId": "hevc",
## codec profile, "avc:main", "avc:high", "hevc:main", or "hevc:main10"
"encode-codecProfile": "hevc:main",
## asynchronous operations number
"encode-async-depth": 4,
## the preset for quality and performance balance, "balanced", "quality", or "speed"
"encode-target-usage": "balanced",
## rate control mode, "CQP" or "VBR"
"encode-rcMode": "VBR",
## quantization value under CQP mode
"encode-qp": 26,
## bitrate value under VBR mode
"encode-bitrate": 15000,
## frame rate numerator
"encode-fps": 30,
## the distance between two adjacent intra frame
"encode-gop": 30,
## maximum number of B-frames between non-B-frames
"encode-max-bFrames": 0
```

Then start the MDSCMRDASample:
```bash
cd scripts\build\Release
.\MDSCMRDASampleApp.exe
```

To read from the published RTSP stream, you can use ffplay in your client machine:
```bash
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38554/screencap0
```
For multiple displays in 1 VM:
```bash
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38554/screencap0
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38554/screencap1
```
For multiple VMs:
```bash
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38554/screencap0
ffplay -x 1920 -y 1080 -rtsp_transport tcp rtsp://${Host_IP}:38555/screencap0
```

## Compile:
Open Developer PowerShell for VS 2022 and run the following commands:
```bash
cd scripts
.\WinBuild.bat
```
The binary can be found in "scripts\build\Release" folder.

## License
MDSCMRDASample module is licensed under the MIT license. Note that the MIT license is only the license for Intel VDI Tool Kit itself, independent of its third-party dependencies, which are separately licensed.

One of the dependencies, FFmpeg, is an open source project licensed under LGPL and GPL. See [FFmpeg Legal](https://www.ffmpeg.org/legal.html). You are solely responsible for determining if your use of FFmpeg requires any additional licenses. Intel is not responsible for obtaining any such licenses, nor liable for any licensing fees due, in connection with your use of FFmpeg.