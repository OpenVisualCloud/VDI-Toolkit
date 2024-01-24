#!/bin/bash

VM_NAME=win2k19
GPU_CARD=0

usage(){
cat << EOF
Usage: ./4_setup_vm_spice_stream.sh [OPTIONS]
Setup env path for qemu-commandline to enable gstreamer HW stream encoder.
Notes: Need to mannual add "<streaming mode='all'/>" for <graphics spice> element as shown in README.md.
Options:
--vm-name=win2k19       set the vm_name
--gpu-card=1            set gpu card number, default is 0
-h|--help               Help, more infomation can be found in README.md
EOF
}

while [[ $# -gt 0 ]];
do
    case "$1" in
        "--vm-name="*)
            VM_NAME=${1##*=}
            echo "VM_NAME=${VM_NAME}"
            shift
            ;;
        "--gpu-card="*)
            GPU_CARD=${1##*=}
            echo "GPU_CARD=$GPU_CARD"
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

echo "Please firstly mannual add "streaming mode='all'" for "graphics spice" element!"

virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=LD_LIBRARY_PATH=/opt/intel/spice/lib:/opt/intel/gst/lib:/opt/intel/gst/lib/gstreamer-1.0:/usr/local/lib:/usr/local/lib64'
virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=LIBVA_DRIVER_NAME=iHD'
virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=LIBVA_DRIVERS_PATH=/usr/lib64/dri'
virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=GST_VAAPI_DRM_DEVICE=/dev/dri/card'"${GPU_CARD}"
virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=GST_GL_GBM_DRM_DEVICE=/dev/dri/card'"${GPU_CARD}"
virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=GST_PLUGIN_PATH=/opt/intel/gst/lib/gstreamer-1.0'
#virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=GST_MESSAGE_DEBUG=all'
#virt-xml "${VM_NAME}" --edit --confirm --qemu-commandline 'env=GST_DEBUG=5'
