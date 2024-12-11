#!/bin/sh

mkdir build
cd build
cmake .. -DPICO_SDK_PATH=`readlink -f ../../pico-sdk/`
make -j 14

