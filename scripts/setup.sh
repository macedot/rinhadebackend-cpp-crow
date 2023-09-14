#!/bin/bash

set -e
set -x

BASEDIR=$(pwd -P)
echo "BASEDIR=$BASEDIR"

# vcpkg for all other dependencies
export VCPKG_FORCE_SYSTEM_BINARIES=1
if [ ! -d "vcpkg" ]; then
    git clone https://github.com/microsoft/vcpkg.git
else
    (cd vcpkg && git pull --ff)
fi
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install

# CrowCpp from git (latest release still have boost dependency)
if [ ! -d "Crow" ]; then
    git clone https://github.com/CrowCpp/Crow.git
fi

(
    cd Crow
    git pull --ff
    rm -rf build && mkdir build && cd build
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCMAKE_TOOLCHAIN_FILE=${BASEDIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
)
