#include "cam.hpp"

//Start Capture data
int cam::start(cam* camera)
{
	camera->init_camera(camera);
	cam_state = DEV_CAM_RUN;
	m_cameraErr = false;
	return 0;
}

//Stop Capture data
int cam::stop()
{
	cam_state = DEV_CAM_GO_PAUSE;
	m_cameraErr = false;
	return 0;
}

//camera construct function
cam::cam(std::string config)
{
	pthread_mutex_init(&get_data_lock, NULL); //pthread lock
    camera_driver_type = 0;		//camera driver type
    fd = 0;						//camera device's fd
    cam_state = DEV_CAM_PAUSED;	//camera status
    m_cameraErr = false;	 	//camera error
    dev_path = config;  		//camera device's path
	buffers = NULL;				//image buff
	img_width = 640;			//image width
	img_height = 480;			//image height
	v4l2buf.index = 0;
	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; //V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2buf.memory = V4L2_MEMORY_MMAP;
}

//init camera class
int cam::init_camera(cam* camera)
{
    // prepare camera
    int cam_ret = 0;
    cam_ret = camera->Prepare(camera->dev_path);
	if (cam_ret < 0) {
		printf("camera prepare Failed !");
        return -1;
	}
    // launch camera
	cam_ret = camera->LauchCamera();
	if (cam_ret < 0) {
		printf("camera lauch Failed !");
        return -1;
	}
    
    // create capture thread
    std::thread* camera_thread = new std::thread(&cam::capture_forever, camera);
	std::cout<<"start capture thread successfully."<< std::endl;
	return 0;
}

//camera prepare
int cam::Prepare(const std::string & cameraName)
{
	// Open camera device
	char *cameraNameChar = const_cast < char *>(cameraName.data());
	fd = open(cameraNameChar, O_RDWR);
	if (fd <= 0) {
		perror("Error opening camera device!\n");
		return -1;
	}
	printf("Open camera device success %d.\n", fd);

	// Check device dirver capabilities
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
		perror("VIDIOC_QUERYCAP Error.\n");
		close(fd); // shutdown camera device
		return -1;
	}

	if (!(cap.capabilities & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE))) {
		fprintf(stderr, "Device is no video capture device\n");
		close(fd); // shutdown camera device
		return -1;
	}

	if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        camera_driver_type = V4L2_CAP_VIDEO_CAPTURE_MPLANE;
		std::cout<<"the camera driver type is: V4L2_CAP_VIDEO_CAPTURE_MPLANE."<< std::endl;
    }
	else if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
	{
        camera_driver_type = V4L2_CAP_VIDEO_CAPTURE;
		std::cout<<"the camera driver type is: V4L2_CAP_VIDEO_CAPTURE."<< std::endl;
    }
	else
	{
        close(fd); // shutdown camera device
        return -1;
    }

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "Device does not support I/O streaming.\n");
		close(fd);
		return -1;
	}

	// Set camera capture input 
	struct v4l2_input inp;
	inp.index = 0;
	inp.type = V4L2_INPUT_TYPE_CAMERA;
	if (ioctl(fd, VIDIOC_S_INPUT, &inp) == -1) 
	{
		printf("VIDIOC_S_INPUT failed! s_input: %d\n", inp.index);
		close(fd); // shutdown camera device
		return -1;
	}

	// get camera sensor type
    camera_sensor_type = getSensorType(fd);
    if(camera_sensor_type == V4L2_SENSOR_TYPE_RAW)
	{
        camera_ispPort = CreateAWIspApi();
    }

	// Set camera capture params 
	struct v4l2_streamparm parms;
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
    else
	{
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = 30; // frame rate
	if (ioctl(fd, VIDIOC_S_PARM, &parms) == -1) 
	{
		printf("VIDIOC_S_PARM failed!\n");
		close(fd);
		return -1;
	}

	// Get and print camera capture params
	memset(&parms, 0, sizeof(struct v4l2_streamparm));
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
    else
	{
        parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}
	if (ioctl(fd, VIDIOC_G_PARM, &parms) == 0) 
	{
		std::cout << "Camera capture framerate is " << parms.parm.capture.timeperframe.denominator 
		<< "/" << parms.parm.capture.timeperframe.numerator << std::endl;
	} 
	else 
	{
		printf("VIDIOC_G_PARM failed!\n");
	}

	// Set and print image capture format
	memset(&fmt, 0, sizeof(fmt));
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.width = img_width;				// image width
        fmt.fmt.pix_mp.height = img_height;				// image height
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21; // YUYV;
        fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    }
	else
	{
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = img_width;					// image width
        fmt.fmt.pix.height = img_height;				// image height
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;    // YUYV
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
    }

	if (ioctl(fd, VIDIOC_S_FMT, &fmt)  < 0) 
	{
		perror("VIDIOC_S_FMT Error.");
		close(fd); // shutdown camera device
		return -1;
	}

	if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0)
	{
		perror("VIDIOC_G_FMT Error.");
		close(fd); // shutdown camera device
		return -1;
	}
	printf(" fmt.type = %d\n",fmt.type);
	printf(" fmt.fmt.pix.width = %d\n",fmt.fmt.pix_mp.width);
	printf(" fmt.fmt.pix.height = %d\n",fmt.fmt.pix_mp.height);
	printf(" fmt.fmt.pix.field = %d\n",fmt.fmt.pix_mp.field);
	printf(" fmt.fmt.pix_mp.num_planes = %d\n",fmt.fmt.pix_mp.num_planes);

	// Prepare Camera device successfully
	std::cout<<"Camera_device:: Prepare successful. \n";
	return 0;
}

//launch camera
int cam::LauchCamera()
{
	// ask for driver to frame buff 
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 3; // size of frame buff is 3
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
    else
	{
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
	}
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &req) < -1) 
	{
		perror("VIDIOC_REQBUFS failed");
		close(fd); // shutdown camera device
		return -1;
	}
	if (req.count < 1) 
	{
		fprintf(stderr, "Insufficient buffer memory on device\n");
		close(fd); // shutdown camera device
		return -1;
	}

	// Mmap for buffers
	buffers = (img_buffer *)calloc(req.count, sizeof(*buffers));
	if (!buffers) 
	{
		fprintf(stderr, "Out of memory\n");
		close(fd); // shutdown camera device
		return -1;
	}

	// transfer the frame buff to physical memory
	struct v4l2_buffer buf;
	for (size_t n = 0; n < req.count; ++n) 
	{
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		}
        else
		{
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		}

		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n;
		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
            buf.length = fmt.fmt.pix_mp.num_planes;
            buf.m.planes =  (struct v4l2_plane *)calloc(buf.length, sizeof(struct v4l2_plane));
        }
		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) 
		{
			perror("VIDIOC_QUERYBUF Error");

			if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
			{
                free(buf.m.planes); // free frame buff
			}
			free(buffers);  // free buffers
			buffers = NULL; // free buffers
			close(fd);		// shutdown camera device
			return -1;
		}

		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
            for(int i = 0; i < fmt.fmt.pix_mp.num_planes; i++)
			{
                buffers[n].length[i] = buf.m.planes[i].length;
                buffers[n].start[i] = mmap(NULL , buf.m.planes[i].length,
                                                   PROT_READ | PROT_WRITE, \
                                                   MAP_SHARED , fd, \
                                                   buf.m.planes[i].m.mem_offset);
				if (buffers[n].start[i] == MAP_FAILED) 
				{
					free(buffers);
					buffers = NULL;
					perror("mmap");
					close(fd);
					return -1;
				}
			}
            free(buf.m.planes); // free frame buff
        }
		else
		{
			buffers[n].length[0] = buf.length;
			buffers[n].start[0] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

			if (buffers[n].start[0] == MAP_FAILED) 
			{
				free(buffers);
				buffers = NULL;
				perror("mmap");
				close(fd);
				return -1;
			}
		}
	}

	// Queue, Exchange buffers with the driver frame buff
	for (size_t n = 0; n < req.count; ++n) 
	{
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		}
        else
		{
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		}
		buf.memory= V4L2_MEMORY_MMAP;
		buf.index = n;

		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
            buf.length = fmt.fmt.pix_mp.num_planes;
            buf.m.planes =  (struct v4l2_plane *)calloc(buf.length, sizeof(struct v4l2_plane));
        }

		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) 
		{

			if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
			{
                free(buf.m.planes);
			}

			if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
			{
				for (int i = 0; i < 3; i++) 
				{
					for(int num = 0; num < fmt.fmt.pix_mp.num_planes; num++)
					{
						munmap(buffers[i].start[num], buffers[i].length[num]);
					}
				}
			}
			else
			{
				for (int i = 0; i < 3; i++) 
				{
					munmap(buffers[i].start[0], buffers[i].length[0]);
				}
			}
			free(buffers);
			buffers = NULL;
			close(fd);
			return -1;

		}
	}
	
	// Starting image capture
	enum v4l2_buf_type type;
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
    else
	{
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}

	if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) 
	{
		std::cout<< "VIDIOC_STREAMON Error."<< std::endl;

		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
			for (int i = 0; i < 3; i++) 
			{
				for(int num = 0; num < fmt.fmt.pix_mp.num_planes; num++)
				{
					munmap(buffers[i].start[num], buffers[i].length[num]);
				}
			}
		}
		else
		{
			for (int i = 0; i < 3; i++) 
			{
				munmap(buffers[i].start[0], buffers[i].length[0]);
			}
		}
		free(buffers);
		buffers = NULL;
		close(fd);
		return -1;
	}

	if(camera_sensor_type == V4L2_SENSOR_TYPE_RAW)
	{
	    camera_ispId = -1;
	    camera_ispId = camera_ispPort->ispGetIspId(0); // camera->camera_index;
	    if(camera_ispId >= 0)
		{
			// starting  capture
	        camera_ispPort->ispStart(camera_ispId);
		}
	}

	// Camera launch successfully
	std::cout<<"Camera_device:: Launch success. \n";
	return 0;
}

//----------------------------------------
long long cam::GrabNewImage(cv::Mat & I)
{
	// judge the camera status
	if( cam_state != DEV_CAM_RUN )
	{
		std::cout<<"the status is DEV_CAM_PAUSE, Please using start() before GrabNewImage(). \n";
		return -1;
	}

	if (!(I.cols == (signed)img_width && I.rows == (signed)img_height)) 
	{
		std::cout<<"the image cols or rows is error, exitting."<<std::endl;
		exit(-3);
	}

	// get image data 
	int res = 0;
	do 
	{
		pthread_mutex_lock(&get_data_lock);
		if (DEV_CAM_RUN == cam_state) 
		{
			if (v4l2buf.index != -1) 
			{
				//获取摄像头的图像数据: 640*480
				const uchar *NV21 = (const uchar *)buffers[v4l2buf.index].start[0];
				NV21ToGray(I, NV21);
			} 
			else 
			{	//假数据,图片像素全部为白色: 640*480
				memset((uchar *) I.ptr(), 255, img_width * img_height);
			}
			res = 1;
		} 
		else 
		{
			res = 0;
		}
		pthread_mutex_unlock(&get_data_lock);

	} while (res == 0);

	// compute timestamps 
	struct timeval time;
	gettimeofday(&time, 0);
	unsigned long long m_time = (unsigned long long)time.tv_sec * 1000000 + time.tv_usec;
	return m_time;
}

//----------------------------------------
long long cam::get_data(cv::Mat & I)
{
	long long ret;

	printf("start get camera's data ------------\n");
	ret = GrabNewImage(I); // obtain image and timestamp
	printf("camera_device :: obtain image and timestamps successfully.\n");

	return ret; // return timestamps
}

//----------------------------------------
void cam::capture_onetime()
{
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
	else
	{
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
	}
	buf.memory = V4L2_MEMORY_MMAP;
	if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
	{
        buf.length = fmt.fmt.pix_mp.num_planes;
        buf.m.planes = (struct v4l2_plane *)calloc(fmt.fmt.pix_mp.num_planes, sizeof(struct v4l2_plane));
    }

	fd_set fds;
	if (!m_cameraErr) 
	{
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
	}

	// camera error and go paused mode
	if (m_cameraErr) 
	{
		usleep(33000);
		v4l2buf.index = -1;
		gettimeofday(&v4l2buf.timestamp, NULL);
		if (cam_state == DEV_CAM_GO_PAUSE) 
		{
			cam_state = DEV_CAM_PAUSED;
		}
	}

	// get data from frame buff
	if (ioctl(fd, VIDIOC_DQBUF, &buf) != 0) 
	{
		perror("VIDIOC_DQBUF Error.\n");
	}

	// exchange data between v4l2buf and frame buff
	memcpy(&v4l2buf, &buf, sizeof(buf));

	// return data to frame buff
	if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) 
	{
		perror("VIDIOC_QBUF Error.\n");
	}
}

//----------------------------------------
void cam::capture_forever()
{
	while (1)
	{
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		}
		else
		{
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
		}
		buf.memory = V4L2_MEMORY_MMAP;
		if(camera_driver_type == V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		{
			buf.length = fmt.fmt.pix_mp.num_planes;
			buf.m.planes = (struct v4l2_plane *)calloc(fmt.fmt.pix_mp.num_planes, sizeof(struct v4l2_plane));
		}

		fd_set fds;
		if (!m_cameraErr) 
		{
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
		}

		// camera error and go paused mode
		if (m_cameraErr) 
		{
			usleep(33000);
			v4l2buf.index = -1;
			gettimeofday(&v4l2buf.timestamp, NULL);
			if (cam_state == DEV_CAM_GO_PAUSE) 
			{
				cam_state = DEV_CAM_PAUSED;
			}
		}

		// get data from frame buff
		if (ioctl(fd, VIDIOC_DQBUF, &buf) != 0) 
		{
			perror("VIDIOC_DQBUF Error.\n");
		}

		// exchange data between v4l2buf and frame buff
		memcpy(&v4l2buf, &buf, sizeof(buf));

		// return data to frame buff
		if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) 
		{
			perror("VIDIOC_QBUF Error.\n");
		}	

		// exit capture thread
		if( cam_state == DEV_CAM_GO_PAUSE )
		{
			break;
		}
	}

	std::cout<<"capture thread exiting."<< std::endl;
}

//----------------------------------------
int cam::getSensorType(int fd)
{
    int ret = -1;
    struct v4l2_control ctrl;
    struct v4l2_queryctrl qc_ctrl;

    if (fd == 0)
	{
        return 0xFF000000;
	}

    memset(&ctrl, 0, sizeof(struct v4l2_control));
    memset(&qc_ctrl, 0, sizeof(struct v4l2_queryctrl));
    ctrl.id = V4L2_CID_SENSOR_TYPE;
    qc_ctrl.id = V4L2_CID_SENSOR_TYPE;

    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &qc_ctrl)){
		std::cout<<"VIDIOC_QUERYCTRL Error.\n";
        return -1;
    }

    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if(ret < 0){
		std::cout<<"VIDIOC_G_CTRL Error.\n";
        return ret;
    }
    return ctrl.value;
}

//----------------------------------------
int cam::NV21ToGray(cv::Mat& I,const uchar *NV21)
{
    unsigned char *src_y = (unsigned char *)NV21;
	uchar *imgPtr = (uchar *) I.ptr();

    if(imgPtr == NULL || NV21 == NULL || img_width <= 0 || img_height <= 0)
    {
        printf(" NV21ToGray incorrect input parameter!\n");
        return -1;
    }

    for(int y = 0;y < img_height;y ++)
	{
        for(int x = 0;x < img_width;x ++)
		{
            int Y = y*img_width + x;
			imgPtr[Y] = src_y[Y];
        }
    }
    return 0;
}





