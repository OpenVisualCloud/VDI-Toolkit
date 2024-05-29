# MultiDisplayScreenCapture Guide
This module contains a library ``MultiDisplayScreenCapture.dll`` and a sample ``MDSCSample.exe`` to capture screen of multiple IDD displays and encode images in h264 format with ffmpeg library. There are different configurations for different usage of the sample.

## Build
Run ``MDSC-build.bat`` to automatically fetch the dependencies and build the binary. The results are in ``bin`` folder.

The dependencies of MDSCSample include ``nlohmann\json.hpp`` and ``ffmpeg-6.1.1-full_build-shared`` and they are downloaded in ``MDSCSample\deps`` folder. With these dependencies, open
 ``MultiDisplayScreenCapture.sln`` and the solution can be built witn default config. And C/C++ Processor Definitions "DUMP_RGBA" can be added in "MDSCSample" project properties to build the MDSCSample with BGRA raw data dumping enabled.

 To enable rtsp streaming, ``mediamtx_v1.6.0_windows_amd64`` is downloaded to run rtsp server. And there are also 1 config file ``MDSCSample.conf`` and 3 scripts ``start_rtsp_server.bat``, ``ffplay_streams.bat``, ``start_rtsp_server.bat`` for sample usage.

## Usage:
### 1. Config Params Explanation
```bash
# 1. capture params:
## capture display with fix fps
"capture-fps": 30,
## capture single display("true") or multiple displays("false")
"capture-single-display": false,
## when capture-single-display is "true", select which display to capture, and if the number is larger than the total number of multi-displays, the default display with number 0 will be captured
"capture-single-display-number": 0,
## the path to save "*.rgba" files when "DUMP_RGBA" is defined when building the sample
"capture-dump-path": "capture_dump"

# 2. encode params:
## encode type, currently only support "ffmpeg-software"
"encode-type": "ffmpeg-software",
## encode bitrate
"encode-bitrate": 3000000,
## encode fps
"encode-fps": 30,
## encode gop size
"encode-gop": 30,
## encode qp value
"encode-qp": 26,
## encode qmin value
"encode-qmin": 10,
## encode qmax value
"encode-qmax": 51,
## encode qcompress value
"encode-qcompress": 0.6,
## the path to save "*.mp4" files when "stream-mode" is "false"
"encode-dump-path": "encode_dump"

# 3. stream params
## rtsp server ip
"rtsp-server-ip": "127.0.0.1",
## rtsp server port
"rtsp-server-port": "8554",
## rtsp stream mode("true") or encode file dump mode("false"),
## the rtsp url is "rtsp://%rtsp-server-ip%:%rtsp-server-port%/screencap%display-number%"
"stream-mode": true
```

### 2. Run binary with scripts
```bash
cd bin
#1. Edit ``MDSCSample.conf`` for different usage.
#2. For stream mode, run ``start_rtsp_server.bat`` to start rtsp server first, no need to execute this step for encode file dump mode, and config can be edited in "mediamtx_v1.6.0_windows_amd64\mediamtx.yml".
start_rtsp_server.bat
#3. Run ``MDSCSample.exe`` to start capture and encode. Finish it with "Ctrl+C" input.
MDSCSample.exe
#4 For stream mode, run ``start_rtsp_client.bat`` to start rtsp client(ffplay), and %clients_num%, %clients_st_id%, %rtsp-server-ip%, "rtsp-server-port" params can be edited according to the usage and the ffplay window size can be edited in ffplay_streams.bat
# e.g. start 2 rtsp clients with url of
# rtsp://127.0.0.1:8554/screencap0 and
# rtsp://127.0.0.1:8554/screencap1
start_rtsp_server.bat
# or
ffplay_streams.bat 2 0 127.0.0.1 8554
# 5 For encode file dump mode, after step #3 is finished, "%encode-dump-path%*.mp4" can be found in "bin" folder.
```

## License
MultiDisplayScreenCapture module is licensed under MIT license. Note that MIT license is only the license for Intel VDI Tool Kit itself, independent of its third-party dependencies, which are separately licensed.

One of the dependecies FFmpeg is an open source project licensed under LGPL and GPL. See https://www.ffmpeg.org/legal.html. You are solely responsible for determining if your use of FFmpeg requires any additional licenses. Intel is not responsible for obtaining any such licenses, nor liable for any licensing fees due, in connection with your use of FFmpeg.