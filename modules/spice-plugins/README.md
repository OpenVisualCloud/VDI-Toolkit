# Spice-plugins
This module includes patches and scripts for spice library to enable HW stream encoder.

Main dependencies: spice, gstreamer.

The ``patches`` folder includes the patches to enable the HW gstreamer-vaapi:jpegenc feature.

The ``scripts`` folder includes the scripts to setup the environment, and compile and install
gstreamer-1.20.3, spice-protocol-0.14.0 and spice-server-0.14.3.

## Patches
### gstreamer
This patch fixes a compile error with recent upstream FFmpeg:

``0001-avviddec-change-AV_CODEC_CAP_AUTO_THREADS-AV_CODEC_C.patch``

This patch disables libjpeg-turbo SIMD acceleration support to avoid jpegdec failure when format is converted to BGRx:

``0002-jpegdec-Disable-libjpeg-turbo-SIMD-acceleration-supp.patch``
### spice-protocol
This patch adds vaapijpeg type for HW encoder:

``0001-Add-HW-encoder-codec-type.patch``
### spice
This patch changes url of submodules spice-common to avoid sync failure:

``0001-Change-url-of-submodules-spice-common.patch``

This patch enables gstreamer-vaapi:jpegenc for spice-server stream encoder:

``0002-Enable-HW-vaapi-jpeg-encoder.patch``

## Scripts
### Install platform
This script is used to install kernel, media and ffmpeg from platform packages.

The path of downloaded and unzipped platform pacakges is needed as param.

Usage of this script shall be:
```bash
./0_install_platform.sh --platform-path="/opt/build/10785-data-center-gpu-c7.4-k5.4.19"
```

###  Install gstreamer
This script is used to compile and install gstreamer.

The installation path is set as /opt/intel/gst.
The usage of this script shall be:
```bash
./1_install_gstreamer.sh
```
**Notes**: Platform packages must be installed first!

### Install spice
This script is used to compile and install spice-protocol-0.14.0 and spice-0.14.3.
The installation path is set as /opt/intel/spice.
The usage of this script shall be:
```bash
./2_install_spice.sh
```
**Notes**: gstreamer-1.20.3 must be installed first!

### Test Spice GST
This script is used to run unit tests for spice-sever stream encoder.
Env path will be set and two unit tests of spice/server:
"test-codecs-parsing" and "test-video-encoders" will be run.
The gpu card number is needed as a param and the default value is 0,
which means the full device path is "/dev/dri/card0".
The usage of this script shall be:
```bash
./3_test_spice_gst.sh --gpu-card=1
```

### Setup VM Spice Stream
This script is used to setup env path for qemu-commandline to enable gstreamer HW stream encoder.
The name of the VM is needed as param.

**Notes**: Need to uncomment 'cgroup_device_acl' in '/etc/libvirt/qemu.conf' and add gpu device.

The gpu card number shall be modified according to the device and it is 1 in below example.
<div align=center><img src="./assets/cgroup_device_acl.PNG"></div>

**Notes**: Need to add spice graphics device and qxl video device in VM.

And need to mannually add "streaming mode='all'" for "graphics spice" element as shown below.
```
vish edit $VM_NAME
```
<div align=center><img src="./assets/StreamingMode.PNG"></div>

**Notes**: Need to install virtio driver in VM.

Download virtio driver from https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.215-2/virtio-win-0.1.215.iso
and copy the iso to the VM to be installed through share folder. Then, attach this ISO on the VM and run virtio-win-guest-tools.exe in the image to complete the installation. After installation, the "Display adapters" should include "Red Hat QXL controller".
<div align=center><img src="./assets/QXLcontroller.PNG"></div>

Finally use this script to setup the env path to enable HW stream encoder.
The name of the VM and the gpu card number are needed as params.
The usage of this script shall be:

```bash
sudo su
./4_setup_vm_spice_stream.sh --vm-name=win2k19 --gpu-card=1
```

Input "y" after confirming each env path edition.

Then start the VM and run virt-viewer, the streaming mode will be valid when playing a video.

GPU usage can be tested through tools of intel-gpu-top or xpu-smi to test if HW gstreamer-vaapi:jpeg encoder is enabled.

Besides, uncomment the lines including 'env=GST_MESSAGE_DEBUG=all' and 'GST_DEBUG=5' in the script to print gst debug logs and run again, "vaapijpegenc" related logs can be found in '/var/log/libvirt/qemu/${VM_NAME}.log'.
