### 实现摄像头的图片和时间戳的抓取，精简重构原来的摄像头抓取代码.
### 近端PC使用方法：
    mkdir build/
    build.sh   : 编译软件包并推送到远端RAM中.
    mkdir image_data/
    getdata.sh : 从远端ARM的 /root/image_data/ 中提取图片和时间戳数据.

### 远端ARM的具体操作方法
    adb shell
    cd /root/image_data/
    rm -rf *
    cd ..
    chmod +x test_camera
    ./test_camera
    按Enter按键实现采集一张以时间戳命名的图片数据，并存入/root/image_data/中.
