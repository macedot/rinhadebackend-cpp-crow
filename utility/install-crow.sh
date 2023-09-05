#!/bin/sh

BUILD_TYPE=$1

if [ -z "$BUILD_TYPE" ]; then
    BUILD_TYPE="Debug"
fi

rm -rf tmp
mkdir tmp
cd tmp

##########################################################
## install module

function install_module () {

    BUILD_TYPE=$1
    MODULE_NAME=$2
    NPROC=$(nproc)

    if [ -z "$NPROC" ]; then
        NPROC=1
    fi

    echo "\n\nINSTALLING MODULE '$MODULE_NAME' ($BUILD_TYPE) using $NPROC threads ...\n\n"

    git clone https://github.com/CrowCpp/$MODULE_NAME
    cd $MODULE_NAME

    mkdir build
    cd build

    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    make install -j $NPROC

    cd ../../
}

##########################################################

install_module $BUILD_TYPE crow

cd ../
rm -rf tmp
