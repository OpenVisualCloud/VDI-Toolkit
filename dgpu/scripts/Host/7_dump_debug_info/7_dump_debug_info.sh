#!/bin/bash

LOGPATH=$(pwd)/dumplogs
if [[ ! -d ${LOGPATH} ]]
then
    mkdir "${LOGPATH}"
else
    rm -rf "${LOGPATH}"
    mkdir "${LOGPATH}"
fi
mkdir "${LOGPATH}"/host
HOST_CONFIG_PATH=${LOGPATH}/host_config
mkdir "${HOST_CONFIG_PATH}"
VIRSH_PATH=${LOGPATH}/virsh
XPUM_PATH=${LOGPATH}/host/xpum
mkdir "${VIRSH_PATH}"
mkdir "${XPUM_PATH}"

GPU_CARD=0 #if there is an iGPU, input param --gpu_card=1 to modify it

usage(){
cat << EOF
Usage: ./7_dump_debug_info.sh [OPTIONS]
Dump Host debug info with this script
Options:
--with-dependency       to install the tools when first time run this script
--gpu-card=0            choose the gpu card id to dump gpu data, default is 0
-h|--help               Help, more infomation can be found in README.md
EOF
}

while [[ $# -gt 0 ]];
do
    case "$1" in
        "--with-dependency")
            install_dependency=1
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

if [[ ${install_dependency} = 1 ]]; then
    sudo yum install -y sysstat
    sudo yum install -y tree
fi

LINUX_DISTRO=$(lsb_release -si)

function check_cputype_dumpinfo() {
    icelake=(8358 6348 8380 6346)
    c_type="cascadelake"
    for i in ${icelake[*]}
    do
        if lscpu | grep "Model name" | grep "$i"
        then
            c_type="icelake"
        fi
    done
}

function sys_state(){
    echo ".......................... sar, top, free, journactl, dmesg .........................."
    echo "........................... here please wait a few seconds ..........................."
    sar -u ALL -r -n DEV 1 5          > "${LOGPATH}"/host/sar_dump.log
    #sar -u All：show avg usage of all cpu；
    #sar -r: show mem satus
    #sar -n DEV 1 5: totally test 5 times with 1s interval
    top -b -n 5                       > "${LOGPATH}"/host/top.log
    #top -b -n 5: test 5 times
    free -m -c 5                      > "${LOGPATH}"/host/free.log
    #free -m -c 5: test 5 times, unit M
    ps aux --sort=-rss | sudo tee "${LOGPATH}"/host/ps_aux.log > /dev/null
    #ps aux --sort=-rss: sorting down processes by memory usage
    journalctl -b 0 | sudo tee "${LOGPATH}"/host/journalctl.log > /dev/null
    #journalctl -b 0: query the journel since current boot
    sudo dmesg -T | sudo tee "${LOGPATH}"/host/dmesg.log > /dev/null
    #dmesg -T: dmesg with human readable timestamp
    sudo cpupower monitor -m Mperf -i 5 | sudo tee "${LOGPATH}"/host/CPU_pwmon.log > /dev/null
    #cpupower monitor -m Mperf -i 5: monitor avg cpupower with 5s time interval
}

function sys_data() {
    echo "............ cpu_type, slabinfo, lspci-t, Gpu-device-map, cmdline ............"
    echo "..... numa, kmod, i915-driver, vainfo, lscpu, lscpu-p, scaling_governor ......"
    check_cputype_dumpinfo
    # cpu_type
    echo $c_type > "$HOST_CONFIG_PATH"/cpu_type
    # slabinfo
    sudo cat /proc/slabinfo | sudo tee "$HOST_CONFIG_PATH"/slabinfo.txt > /dev/null
    # GPU address map
    lspci -t > "$HOST_CONFIG_PATH"/lspci-t.txt
    for i in $(lspci | grep -iE "vga|display" | grep "Intel Corporation Device" | awk '{print $1}')
    do
        lspci -s "$i" -vvv >> "$HOST_CONFIG_PATH"/lspci_card_info.txt
    done
    tree /sys/dev/char |grep drm > "$HOST_CONFIG_PATH"/GPU-device-map.txt
    lspci | grep -iE "vga|display" | grep "Intel Corporation Device" | cut -d ":" -f 1 > "$HOST_CONFIG_PATH"/PCI.txt
    # System cmdline
    cat /proc/cmdline > "$HOST_CONFIG_PATH"/cmdline.txt
    # # GPU numa info
    for j in $(lspci | grep 56c0 | awk '{print $1}')
    do
        lspci -s "$j" -vvv >> "$HOST_CONFIG_PATH"/numa_info.txt
    done
    # check i915 driver
    if [[ ${LINUX_DISTRO} == "CentOS" ]]
    then
        i915_ko_path="extra/ukmd/i915.ko"
    elif [[ ${LINUX_DISTRO} == "Debian" ]]
    then
        sudo ln -s /sbin/modinfo /usr/bin/modinfo
        i915_ko_path="updates/ukmd/i915.ko"
    fi
    if [[ "$(modinfo i915 | grep filename)" =~ ${i915_ko_path} ]]
    then
        echo "i915 driver check: passed" >> "$HOST_CONFIG_PATH"/i915_driver_check.txt
    else
        echo "i915 driver check: failed" >> "$HOST_CONFIG_PATH"/i915_driver_check.txt
        modinfo i915 | grep filename >> "$HOST_CONFIG_PATH"/i915_driver_check.txt
    fi
    echo "i915 driver RHWO patch check:" >> "$HOST_CONFIG_PATH"/i915_driver_check.txt
    cat /sys/kernel/debug/dri/"${GPU_CARD}"/i915_workarounds >> "$HOST_CONFIG_PATH"/i915_driver_check.txt
    # media-driver version
    export LIBVA_DRIVERS_PATH=/usr/lib64/dri
    export LIBVA_DRIVER_NAME=iHD
    vainfo 2>/dev/null |grep version > "$HOST_CONFIG_PATH"/media_driver_version.txt
    # lscpu
    lscpu > "$HOST_CONFIG_PATH"/lscpu.txt
    lscpu -p > "$HOST_CONFIG_PATH"/lscpu-p.txt
    for i in $(lscpu -p | grep -v "#")
    do
        echo "cpu-$(echo "$i" | awk -F ',' '{print$1}')::numa-$(echo "$i" | awk -F ',' '{print$4}')" >> "$HOST_CONFIG_PATH"/cpu_numa_info.txt
    done
    # get kmod version
    if [[ ${LINUX_DISTRO} == "CentOS" ]]
    then
        rpm -qa | grep kmod-ukmd >> "$HOST_CONFIG_PATH"/kmod_version.txt
    elif [[ ${LINUX_DISTRO} == "Debian" ]]
    then
        dpkg -l | grep kmod-ukmd >> "$HOST_CONFIG_PATH"/kmod_version.txt
    fi
    # uname -r
    uname -r > "$HOST_CONFIG_PATH"/uname-r.txt
    # scaling_governor
    cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > "$HOST_CONFIG_PATH"/scaling_governor

    sys_state
}

function xpum_data(){
    echo "........ xpu-smi discovery, stats, dump ........"
    xpu-smi discovery > "${XPUM_PATH}"/xpum-list.txt
    GPUPaddr=$(xpu-smi discovery | grep -oe "[0-9a-f]\{4\}\:[0-9a-f]\{2\}\:[0-9a-f]\{2\}\.[0-9a-f]")
    mapfile -t GPUArr < <(echo "$GPUPaddr")
    GPUNum=${#GPUArr[@]}
    echo "$GPUNum GPUs detected by XPUM..."
    checkstr=0

    for ((i=1;i<36;i++)); do checkstr=$checkstr,$i; done

    for ((i=0;i<GPUNum;i++));
    do
        xpu-smi discovery -d $i > "${XPUM_PATH}"/xpum-disc-card$i.txt
        xpu-smi stats -d $i > "${XPUM_PATH}"/xpum-stat-card$i.txt
        xpu-smi dump -d $i -i 1 -n 5 -m $checkstr > "${XPUM_PATH}"/xpum-dump-card$i.txt
    done
}

function gpu_data(){
    echo "............i915_error_state, i915_gem_objects, i915_engine_info, intel_gpu_top..............."
    #system GPU data

    card=0
    pci=0
    < "$HOST_CONFIG_PATH"/PCI.txt
    while IFS= read -r i;
    do
        if [[ ${pci} = "${i}" ]]
        then
            continue
        else
            pci=${i}
            echo "new pci ${pci}"
        fi
        cat /sys/kernel/debug/dri/"$GPU_CARD"/i915_error_state > "${LOGPATH}"/host/i915_error_state_"$card".log
        cat /sys/kernel/debug/dri/"$GPU_CARD"/i915_gem_objects > "${LOGPATH}"/host/i915_gem_objects_"$card".log
        cat /sys/kernel/debug/dri/"$GPU_CARD"/i915_engine_info > "${LOGPATH}"/host/i915_engine_info_"$card".log
        abs=$((card+128))
        if [[ $TESTSUITE != "dumpinfo" ]]
        then
            sudo ./intel_gpu_top drm:/dev/dri/renderD"$abs" -o "${LOGPATH}"/host/GPUTOP-renderD"$abs".json -J &
        fi
        card=$((card+1))
        GPU_CARD=$((GPU_CARD+1))
    done < "$HOST_CONFIG_PATH"/PCI.txt
    pkill -kill intel_gpu_top

    xpum_data
}

function vm_data() {
    virsh list --all > "${VIRSH_PATH}"/virsh-VM-list.txt
    vmStr=$(virsh list --all --name)
    mapfile -t vmList < <(echo "$vmStr")
    vmNum=${#vmList[@]}
    echo "$vmNum VMs detected..."
    for ((i=0; i<vmNum; i++));
    do
        vmName=${vmList[i]}
        virsh dumpxml "$vmName" > "${VIRSH_PATH}"/virsh-"$vmName".xml
    done
}

sys_data
gpu_data
vm_data