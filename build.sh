#!/bin/bash

rm -fr build/ 2>/dev/null
mkdir build  2>/dev/null
cd build

cmake .. "-DCMAKE_TOOLCHAIN_FILE=/home/thiago/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    && make

cd ..
