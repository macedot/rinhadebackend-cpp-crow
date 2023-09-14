#!/bin/bash

set -e
set -x

BASEDIR=$(pwd -P)
echo "BASEDIR=$BASEDIR"

# build the app
if [ ! -d "build" ]; then
    # build the app
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${BASEDIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build .
else
    cd build
    cmake --build .
fi
