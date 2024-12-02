### How to install windows ffmpeg library

#### 1. Install msys2
Download the msys2 installer from https://www.msys2.org/, install msys2 and launch a terminal for the UCRT64 environment. Install dependencies for ffmpeg.
```
# Set env in windows environment variable: C:\Program Files\msys64\mingw64\bin
# pacman -S mingw-w64-x86_64-toolchain
# pacman -S base-devel
# pacman -S yasm nasm gcc
# pacman -S pkg-config
```

#### Install ffmpeg
launch a terminal for the UCRT64 environment.
```
# wget https://ffmpeg.org/releases/ffmpeg-7.1.tar.gz
# tar xzvf ffmpeg-7.1.tar.gz
# cd ffmpeg-7.1
# ./configure --prexfix=./ffmpeg_install --enable-shared --disable-static --enable-x86asm --enable-gpl --enable-version3
# make -j
# make install
```
#### Set environment variable
- Set MSYS2_HOME in env variable in windows system control panel: MSYS2_HOME -> C:\Program Files\msys64\home\Administrator
