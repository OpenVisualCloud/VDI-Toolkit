#!/bin/bash

usage(){
cat << EOF
Usage: 1_install_gstreamer.sh
Scripts to compile and install gstreamer-1.20.3.
Notes: Platform packages must be installed first!
Options:
-h|--help               Help, more infomation can be found in README.md
EOF
}

while [[ $# -gt 0 ]];
do
    case "$1" in
        "-h"|"--help")
            usage
            exit 0
            ;;
        *)
            echo "Wrong parameter!"
            exit 1
            ;;
    esac
done

SRC_PATH=$(pwd)/src
if [[ ! -d ${SRC_PATH} ]];then
    mkdir -p "${SRC_PATH}"
fi
PATCH_PATH=$(pwd)/../patches/gstreamer

sudo yum install -y -q epel-release centos-release-scl wget gcc-c++ make libcurl-devel zlib-devel git autoconf

sudo yum install -y ninja-build glib2-devel flex bison libglib2.0-dev tar wge gcc-g++ ca-certificates pkg-config \
    libX11-devel libXv-devel libXt-devel alsa-lib-devel libpango1.0-dev pango-devel libtheora-devel \
    libvisual-devel libegl1-mesa mesa-libGL-devel \
    gdk-pixbuf2-devel libjpeg-turbo-devel libpng-devel zlib-devel libsoup-devel libvpx-dev \
    openssl libcurl-devel librtmp-devel libde265-devel devtoolset-9 \
    wget tar gcc-c++ glib2-devel bison flex  libva-drm systemd-devel gobject-introspection-devel

# add fusion source
sudo yum install -y -q dnf libdnf-devel
dnf install -y https://download1.rpmfusion.org/free/el/rpmfusion-free-release-7.noarch.rpm
dnf install -y https://download1.rpmfusion.org/nonfree/el/rpmfusion-nonfree-release-7.noarch.rpm

sudo yum install -y devtoolset-9

sudo mkdir -p /opt/intel/gst

export CMAKE_PREFIX_PATH=/opt/intel/gst/lib/cmake

# build cmake
if [[ -d $(pwd)/cmake-3.21.3 ]];then
    rm -rf cmake-3.21.3
fi
CMAKE_REPO=https://cmake.org/files
cd "${SRC_PATH}" || exit
wget -O - ${CMAKE_REPO}/v3.21/cmake-3.21.3.tar.gz | tar xz
cd cmake-3.21.3 || exit
./bootstrap --prefix=/usr/local --system-curl
make -j"$(nproc)"
sudo make install
sudo make install DESTDIR=/opt/intel/gst

export LD_LIBRARY_PATH=/opt/intel/gst/lib:/usr/local/lib:${LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=/opt/intel/gst/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:${PKG_CONFIG_PATH}

# install python3.7.9 as gstreamer 1.20.3 needed.
if [[ -d $(pwd)/Python-3.7.9 ]];then
    rm -rf Python-3.7.9
fi
sudo yum -y install openssl-devel bzip2-devel libffi-devel xz-devel
cd "${SRC_PATH}" || exit
wget https://www.python.org/ftp/python/3.7.9/Python-3.7.9.tgz
tar xvf Python-3.7.9.tgz
cd Python-3.7*/
( source /opt/rh/devtoolset-9/enable
./configure --enable-optimizations --enable-shared
sudo make install DESTDIR=/opt/intel/gst
sudo make install )

# Build gstreamer
sudo yum install -y ninja-build glib2-devel flex bison libglib2.0-dev tar wge gcc-g++ ca-certificates pkg-config \
    libX11-devel libXv-devel libXt-devel alsa-lib-devel libpango1.0-dev pango-devel libtheora-devel \
    libvisual-devel libegl1-mesa mesa-libGL-devel \
    gdk-pixbuf2-devel libjpeg-turbo-devel libpng-devel zlib-devel libsoup-devel libvpx-dev \
    openssl libcurl-devel librtmp-devel libde265-devel devtoolset-9 \
    wget tar gcc-c++ glib2-devel bison flex  libva-drm systemd-devel gobject-introspection-devel
# provided by bck: libdrm-devel libva

# build meson
if [[ -d $(pwd)/meson ]];then
    rm -rf meson
fi
MESON_REPO=https://github.com/mesonbuild/meson
cd "${SRC_PATH}" || exit
git clone ${MESON_REPO}; \
cd meson || exit; \
git checkout 0.61.0; \
sudo python3 setup.py install;

# build nasm-2.13.03
if [[ -d $(pwd)/nasm-2.13.03 ]];then
    rm -rf nasm-2.13.03
fi
NASM_REPO=http://www.nasm.us/pub/nasm/releasebuilds/2.13.03/nasm-2.13.03.tar.xz
cd "${SRC_PATH}" || exit
wget ${NASM_REPO}
tar -xf nasm-2.13.03.tar.xz
cd nasm-2.13.03 || exit
./configure --prefix=/usr
make
sudo make install

# build vpx-1.7.0
if [[ -d $(pwd)/libvpx-1.7.0 ]];then
    rm -rf libvpx-1.7.0
fi
VPX_REPO=https://github.com/webmproject/libvpx/archive/v1.7.0/libvpx-1.7.0.tar.gz
cd "${SRC_PATH}" || exit
wget -O - ${VPX_REPO} | tar xz
cd libvpx-1.7.0 || exit
mkdir libvpx-build
cd libvpx-build || exit
../configure --prefix=/opt/intel/gst \
            --enable-shared  \
            --disable-static
sudo make install
sudo make install DESTDIR=/opt/intel/gst

git config --local user.email "noname@example.com"
git config --local user.name "no name"

# build gst-core
if [[ -d $(pwd)/gstreamer ]];then
    rm -rf gstreamer
fi
GSTCORE_REPO=https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd "${SRC_PATH}" || exit
git clone ${GSTCORE_REPO}
cd gstreamer || exit
git checkout 1.20.3 -b dev_1.20.3
git am "${PATCH_PATH}"/*.patch
source /opt/rh/devtoolset-9/enable && \
meson build \
--libdir=/opt/intel/gst/lib --libexecdir=/opt/intel/gst/lib \
--prefix=/opt/intel/gst --buildtype=release -Dpackage-origin="https://gitlab.freedesktop.org/gstreamer" \
-Dexamples=disabled -Dtests=disabled -Ddoc=disabled -Dintrospection=disabled \
-Dgtk_doc=disabled -Dpython=disabled  -Dugly=enabled \
-Dcustom_subprojects="gst-plugins-base,gst-plugins-good,gst-plugins-bad,gst-plugins-ugly" \
-Dlibsoup:sysprof=disabled -Dgpl=enabled -Dlibav=enabled -Dgst-plugins-good:vpx=enabled \
-Dx264:asm=disabled -Dgst-plugins-ugly:x264=enabled -Dlibnice=disabled \
-Ddevtools=disabled -Dges=disabled -Drtsp_server=disabled -Dtls=disabled -Dqt5=disabled \
-Dvaapi=enabled -Dgstreamer-vaapi:with_drm=yes -Dgstreamer-vaapi:with_x11=no \
-Dgstreamer-vaapi:with_glx=no -Dgstreamer-vaapi:with_wayland=no -Dgstreamer-vaapi:with_egl=no
cd build || exit
ninja -j"$(nproc)"
sudo ninja install