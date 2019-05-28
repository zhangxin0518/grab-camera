#!/bin/bash

cd image_data
rm -rf *
adb pull /root/image_data/
ls
cd ..

