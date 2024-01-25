#!/bin/bash

PLATFORM_PATH=/opt/build/10785-data-center-gpu-c7.4-k5.4.19
usage(){
cat << EOF
Usage: ./0_install_platform.sh [OPTIONS]
install kernel, media and ffmpeg from platform package
Options:
--platform-path=""      The downloaded and unzipped platform pacakge
-h|--help               Help, more infomation can be found in README.md
EOF
}

while [[ $# -gt 0 ]];
do
    case "$1" in
        "--platform-path="*)
            PLATFORM_PATH=${1##*=}
            echo "PLATFORM_PATH=$PLATFORM_PATH"
            shift
            ;;
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

cd "${PLATFORM_PATH}" || exit
echo -e "\nn\ny\nn\nn\nn\nn\ny\nn" | sudo ./install-sg2.sh

sudo yum install -y qemu-kvm qemu-img libvirt virt-install libvirt-client virt-manager virt-viewer