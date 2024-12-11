#!/bin/bash -ex

PREBUILD_FLAG=$1

build_gitv2()
{
    OS=$(awk -F= '/^NAME/{print $2}' /etc/os-release)
    if [ "${OS}" == \""CentOS Linux"\" ];then
        sudo yum -y install curl-devel expat-devel gettext-devel openssl-devel zlib-devel perl-ExtUtils-MakeMaker
    elif [ "${OS}" == \""CentOS Stream"\" ];then
        sudo dnf -y install curl-devel expat-devel gettext-devel openssl-devel zlib-devel perl-ExtUtils-MakeMaker
    elif [ "${OS}" == \""Ubuntu"\" ];then
        sudo apt install libcurl4-openssl-dev libexpat1-dev gettext libssl-dev zlib1g-dev libperl-dev make
    fi
    if [ ! -d "./git-2.21.0" ];then
        wget https://github.com/git/git/archive/v2.21.0.tar.gz -O git-2.21.0.tar.gz
        tar xzvf git-2.21.0.tar.gz
    fi
    cd git-2.21.0/
    make prefix=/usr/local/git all -j$(nproc)
    sudo make prefix=/usr/local/git install
    export PATH=/usr/local/git/bin:$PATH
    cd ../
}
build_grpc()
{
    if [ ! -d "./grpc" ];then
        git clone --recurse-submodules -b v1.62.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
    fi
    cd grpc
    if [ ! -d "./install" ];then
        mkdir install
    fi
    mkdir -p cmake/build
    cd cmake/build
    cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR} \
      ../..
    make -j$(nproc)
    sudo make install
    cd ../../..
}

build_external()
{
    mkdir -p ../../external && cd ../../external
    build_gitv2
    build_grpc
    cd ../Scripts/Linux
}

build_host()
{
    if [ ! -d "./build" ];then
        mkdir build
    fi
    cd build
    cmake .. -DCMAKE_PREFIX_PATH=${GRPC_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DVPL_SUPPORT=OFF -DFFMPEG_SUPPORT=ON -DENABLE_TRACE=OFF
    make -j$(nproc)
}

export GRPC_INSTALL_DIR=${PWD}/../../external/grpc/install
export PATH="${GRPC_INSTALL_DIR}/bin:$PATH"



if [ "${PREBUILD_FLAG}" == "y" ] ; then
    build_external
fi
build_host
