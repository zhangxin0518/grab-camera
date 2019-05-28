#!/bin/bash

cd build
rm -rf *
cmake ..
make -j4

cp test_camera ../

adb push test_camera /root/


