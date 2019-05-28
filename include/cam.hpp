#ifndef _NEWCAM_HPP_
#define _NEWCAM_HPP_

extern "C" {
#include "AWIspApi.h"
#include "sunxi_camera_v2.h"
}

#include <string>
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <thread>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>

#define DEV_CAM_RUN (1)		    // run status 
#define DEV_CAM_GO_PAUSE (2)	// go-pause status
#define DEV_CAM_PAUSED (3)	    // pause status


// image buff
struct img_buffer {
	void* start[3];
	size_t length[3];
};


// picture features
struct _img_feature_t{
	float x;
	float y;
};


// camera_grab class
class cam {
    public:
        cam(std::string config);  
        ~cam(){};

        int start(cam* camera);                     // start camera device 
        int stop();                                 // stop camera device
        long long GrabNewImage(cv::Mat & I);        // grab new image and timestamp
        long long get_data(cv::Mat &I);             // only used as test
      
    private:
        int Prepare(const std::string & cameraName);// prepare driver
        int LauchCamera();                          // launch camera
        void capture_onetime();                     // capture data only one time
        void capture_forever();                     // capture data forever
        int init_camera(cam *camera);               // init camera driver
        int  getSensorType(int fd);
        int  NV21ToGray(cv::Mat& I,const uchar *NV21);
  
    public:
        pthread_mutex_t get_data_lock;
        AWIspApi *camera_ispPort;   //相机驱动API
        int camera_ispId;           //相机驱动Id
        int camera_sensor_type;     //相机传感器类型
        int camera_driver_type;     //相机驱动类型
        int img_width;              //图像宽度
        int img_height;             //图像高度
        int fd;                     //相机设备的fd
        img_buffer* buffers;        //图像缓存
        struct v4l2_format fmt;     //设置视频设备捕获格式
        char cam_state;             //相机状态
        struct v4l2_buffer v4l2buf; //本地缓存
        bool m_cameraErr;           //相机错误标志位
        std::string dev_path;       //相机设备文件路径
};

#endif