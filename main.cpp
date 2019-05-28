#include "cam.hpp"
#include <fstream>

// create image and timestamp
cv::Mat image(480,640,CV_8UC1);
long long timestamps;

int main(int argc, char** argv)
{
    // create camera object
    cam* camera = new cam("/dev/video0");

    // start camera device 
    camera->start(camera);

    //循环获取图像和时间戳数据
    while(1)
    {
        int c = getchar();

        if( c ==32 )
        {
            //获取图像和时间戳数据
            timestamps = camera->GrabNewImage(image);
            std::cout<<"The timestamps is: "<< timestamps<< std::endl;
            std::string str;
            str = "/root/image_data/" + std::to_string(timestamps) + ".bmp";
            cv::imwrite( str,image );
        }
        else if( c==97 )
        {
            camera->start(camera);
        }
        else if( c==98 )
        {
            camera->stop();
        }  

    }

    return 0;
}
