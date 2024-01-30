#!/bin/bash -ex

SRC_PATH="$PWD"/..

build_oneVPL()
{
    cd "$SRC_PATH"
    oneVPLDir="oneVPL"
    if [[ ! -d "$oneVPLDir" ]];then
        git clone https://github.com/intel/libvpl.git "$oneVPLDir"
    fi
    cd "$oneVPLDir"
    git checkout v2023.3.1
    git am --ignore-whitespace ../oneVPL-patches/0001-Add-share-memory-file-process-in-sample-encode.patch
    sudo script/bootstrap
    # shellcheck disable=SC1091
    source /opt/rh/devtoolset-7/enable
    script/build
}

build_console()
{
    buildDir="build"
    cd "$SRC_PATH"/console
    if [[ ! -d "$buildDir" ]];then
        mkdir "$buildDir"
    fi
    cd "$buildDir"
    cmake .. && make
}

build_host()
{
    build_oneVPL
    build_console
}

build_host