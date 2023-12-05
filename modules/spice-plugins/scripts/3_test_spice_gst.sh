GPU_CARD=0 #if there is an iGPU, input param --gpu_card=1 to modify it
usage(){
cat << EOF
Usage: ./3_test_spice_gst.sh [OPTIONS]
unit test for spice-sever stream encoder
Options:
--gpu-card=0            choose the gpu card id to dump gpu data, default is 0
-h|--help               Help, more infomation can be found in README.md
EOF
}

while [[ $# -gt 0 ]];
do
    case "$1" in
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

SPICE_PATH=`pwd`/src/spice/
TEST_PATH=${SPICE_PATH}/server/tests
if [[ ! -d ${TEST_PATH} ]];then
    echo "please compile and install spice first!"
    exit 1
fi
cd ${TEST_PATH}
export LD_LIBRARY_PATH=/opt/intel/gst/lib:/opt/intel/gst/lib/gstreamer-1.0:/usr/local/lib:/usr/local/lib64
export LIBVA_DRIVERS_PATH=/usr/lib64/dri
export LIBVA_DRIVER_NAME=iHD
export GST_VAAPI_DRM_DEVICE=/dev/dri/card${GPU_CARD}
export GST_GL_GBM_DRM_DEVICE=/dev/dri/card${GPU_CARD}
./test-codecs-parsing
./test-video-encoders