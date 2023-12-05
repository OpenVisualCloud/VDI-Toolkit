usage(){
cat << EOF
Usage: 2_install_spice.sh
Scripts to compile and install spice.
Notes: gstreamer-1.20.3 must be installed first!
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

SRC_PATH=`pwd`/src
if [[ ! -d ${SRC_PATH} ]];then
    mkdir -p ${SRC_PATH}
fi
SPICE_SERVER_PATCH_PATH=`pwd`/../patches/spice/
SPICE_PROTOCOL_PATCH_PATH=`pwd`/../patches/spice-protocol

export LD_LIBRARY_PATH=/opt/intel/gst/lib:/opt/intel/gst/lib/gstreamer-1.0:${LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=/opt/intel/gst/lib/pkgconfig:${PKG_CONFIG_PATH}

git config --local user.email "noname@example.com"
git config --local user.name "no name"

# build spice-protocal
SPICE_PROTOCOL_REPO=https://gitlab.freedesktop.org/spice/spice-protocol.git
cd ${SRC_PATH}
if [[ -d `pwd`/spice-protocol ]];then
    rm -rf spice-protocol
fi
git clone ${SPICE_PROTOCOL_REPO}
cd spice-protocol
git checkout v0.14.0 -b dev_v0.14.0
git am ${SPICE_PROTOCOL_PATCH_PATH}/*.patch
./autogen.sh --prefix=/opt/intel/spice && sudo make install

sudo yum install -y pkgconfig glib2-devel opus-devel pixman-devel \
    alsa-lib-devel libjpeg-turbo-devel libjpeg-devel nss asciidoc \
    cyrus-sasl-devel libcacard-devel lz4-devel libtool libasan \
    python-six python-pyparsing python3-six python3-pyparsing \
    glib-networking gdk-pixbuf2-devel celt051-devel openssl-devel
export PYTHON=python3
pip3.7 install six
pip3.7 install pyparsing

# build spice-server
SPICE_SERVER_REPO=https://gitlab.freedesktop.org/spice/spice.git
cd ${SRC_PATH}
if [[ -d `pwd`/spice ]];then
    rm -rf spice
fi
git clone ${SPICE_SERVER_REPO}
cd spice
git checkout v0.14.3 -b dev_v0.14.3
git am ${SPICE_SERVER_PATCH_PATH}//*.patch
export C_INCLUDE_PATH=/opt/intel/spice/include/spice-1
. /opt/rh/devtoolset-7/enable && \
SPICE_PROTOCOL_CFLAGS="-I/opt/intel/spice/include/spice-1" \
./autogen.sh --prefix=/opt/intel/spice -localstatedir=/opt/intel/spice/var \
--sysconfdir=/opt/intel/spice/etc --libdir=/opt/intel/spice/lib
make -j$(nproc)
sudo make install