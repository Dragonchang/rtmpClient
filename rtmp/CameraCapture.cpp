#include"CameraCapture.h"

/*****************************************************************
*
* function��CameraCapture
* ���ã����캯��
*
*****************************************************************/
CameraCapture::CameraCapture()
{
	mCapturethread = new NThread();
	mCaptureHandler = new CameraCaptureHandler(mCapturethread->getLooper(), this);
	mCameraDeviceIndex = 2;
	/*
	mCaptureWidth = 848;
	mCaptureHeight = 480;
	mDisplayWidth = 848;
	mDisplayHeight = 480;
	mFramerate = 26;
	*/
	mCaptureWidth = 640;
	mCaptureHeight = 480;
	mDisplayWidth = 640;
	mDisplayHeight = 480;
	mFramerate = 30;
	mFlipMethod = 0;
	mPushRtmp = new PushRtmp(mDisplayWidth, mDisplayHeight, mFramerate, mCameraDeviceIndex);
}

/*****************************************************************
*
* function��~CameraCapture
* ���ã���������
*
*****************************************************************/
CameraCapture::~CameraCapture()
{
	if (mVideoCapture != NULL) {
		if (mVideoCapture->isOpened())
		{
			mVideoCapture->release();
		}
		delete mVideoCapture;
		mVideoCapture = NULL;
	}
}

/*****************************************************************
*
* function��startCameraCapture
* ����doCapture����ʼץȡ����ͷ�ź�
*
*****************************************************************/
void CameraCapture::startCameraCapture()
{
	int isSuccess = openCamera();
	if (isSuccess == FAILURE)
	{
		printf("openCamera failed!");
	}
	Message* message = Message::obtain(CameraCaptureHandler::CAPTURE_MESSAGE);
	mCaptureHandler->sendMessage(message);
}

/*****************************************************************
*
* function��openCamera
* ���ã�
*
*****************************************************************/
int CameraCapture::openCamera()
{
	try
	{
		if (isCISCamera())
		{
			printf("openCamera index 0\n");
			string pipeline = gstreamerPipeline();
			mVideoCapture = new VideoCapture(pipeline, CAP_GSTREAMER);
		}
		else 
		{
			printf("openCamera index 1\n");
			//mVideoCapture = new VideoCapture("/home/deepblue/zfl/test3.mp4");
			
			mVideoCapture = new VideoCapture(mCameraDeviceIndex);
			mVideoCapture->set(cv::CAP_PROP_FRAME_WIDTH, mCaptureWidth);
			mVideoCapture->set(cv::CAP_PROP_FRAME_HEIGHT, mCaptureHeight);
			mVideoCapture->set(cv::CAP_PROP_FPS, mFramerate);
		}
		if (isCameraOpen())
		{
			int inWidth = mVideoCapture->get(cv::CAP_PROP_FRAME_WIDTH);
			int inHeight = mVideoCapture->get(cv::CAP_PROP_FRAME_HEIGHT);
			int fps = mVideoCapture->get(cv::CAP_PROP_FPS);
			int nframes = mVideoCapture->get(cv::CAP_PROP_FRAME_COUNT);
			printf("openCamera success inWidth: %d, inHeight: %d,fps:%d, nframes:%d\n", inWidth, inHeight, fps, nframes);
			//��ʼ����ʽת��������
			m_Vsc = sws_getContext(mCaptureWidth, mCaptureHeight, AV_PIX_FMT_BGR24,  //Դ     ���ߡ����ظ�ʽ
				mDisplayWidth, mDisplayHeight, AV_PIX_FMT_YUV420P,//Ŀ��   ���ߡ����ظ�ʽ
				SWS_BICUBIC,  //�ߴ�仯ʹ���㷨
				0, 0, 0);
			if (m_Vsc == NULL) {
				printf("��ʼ����ʽת��������ʧ��\n");
				releaseCamera();
				return false;
			}
			m_vpts = 0;
			return SUCCESS;
		}
		else {
			printf("openCamera failed\n");
			releaseCamera();
		}
	}
	catch (exception &ex)
	{
		releaseCamera();	
	}
	return FAILURE;
}

/*****************************************************************
* name:isCISCamera
* function:�Ƿ��ǰ�������ͷ
*
*****************************************************************/
bool CameraCapture::isCISCamera()
{
	//if (mCameraDeviceIndex == 3)
	//{
		//return true;
	//}
	return false;
}


/*****************************************************************
* name:isCameraOpen
* function:����ͷ�Ƿ��
*
*****************************************************************/
bool CameraCapture::isCameraOpen()
{
	if (mVideoCapture != NULL && mVideoCapture->isOpened())
	{
		return true;
	}
	return false;
}

/*****************************************************************
* name:releaseCamera
* function:�ͷ���Դ
*
*****************************************************************/
void CameraCapture::releaseCamera()
{
	if (mVideoCapture != NULL)
	{
		if (mVideoCapture->isOpened()) {
			mVideoCapture->release();
		}
		delete mVideoCapture;
		mVideoCapture = NULL;
	}
	if (m_Vsc != NULL)
	{
		sws_freeContext(m_Vsc);
		delete m_Vsc;
		m_Vsc = NULL;
	}
	
}
/*****************************************************************
* name:releaseAVFrame
* function:releaseAVFrame
*
*****************************************************************/
void CameraCapture::releaseAVFrame(AVFrame *avframe)
{
	if (avframe != NULL)
	{
		av_frame_free(&avframe);
		avframe = NULL;
	}
}
/*****************************************************************
* name:cvmatToAvframe
* function:Mat ת AVFrame 
*
*****************************************************************/
AVFrame* CameraCapture::cvmatToAvframe(Mat* image)
{
	if (image != NULL && !image->empty())
	{
		AVFrame *avframe = av_frame_alloc();
		if (avframe != NULL) 
		{
			int width = image->cols;
			int height = image->rows;
			printf("cvmatToAvframe inWidth: %d, height: %d\n", width, height);
			avframe->format = AV_PIX_FMT_YUV420P;
			avframe->width = width;
			avframe->height = height;
			//h264����
			avframe->pts = m_vpts;
			m_vpts++;
			int ret = av_frame_get_buffer(avframe, 32);
			if (ret != 0) {
				char buf[1024] = { 0 };
				av_strerror(ret, buf, sizeof(buf) - 1);
				printf("cvmatToAvframe error buf: %s\n", buf);
				releaseAVFrame(avframe);
				return NULL;
			}

			///rgb to yuv
			//��������ݽṹ
			uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
			indata[0] = image->data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			insize[0] = image->cols * image->elemSize();//һ�У������ݵ��ֽ���
			int h = sws_scale(m_Vsc, indata, insize, 0, image->rows, //Դ����
				avframe->data, avframe->linesize);
			if (h <= 0) {
				printf("cvmatToAvframe error (h <= 0\n");
				releaseAVFrame(avframe);
				return NULL;
			}
		}
		else {
			printf("av_frame_alloc failed\n");
			return NULL;
		}
		return avframe;
	} 

}

/*****************************************************************
* name:doCapture
* function:ʹ��opencvץȡ����ͷ�ź�
*
*****************************************************************/
void CameraCapture::doCapture()
{
	if (!isCameraOpen())
	{
		printf("doCapture with close camera\n");
		int isSuccess = openCamera();
		if (isSuccess == FAILURE)
		{
			printf("openCamera failed!\n");
			//sleep for reduce CPU time 
			sleep(2);
			return;
		}
	}
	Mat frame;
	if (!mVideoCapture->grab())
	{
		printf("Grabs the next frame from capturing device is empty\n");
		return;
	}
	if (!mVideoCapture->retrieve(frame))
	{
		printf("Decodes and returns the grabbed video frame failed\n");
		return;
	}
	mPushRtmp->pushRtmp(cvmatToAvframe(&frame));
}

/*****************************************************************
*
* function��gstreamerPipeline
* ���ã�
*
*****************************************************************/
string CameraCapture::gstreamerPipeline()
{
	return "nvarguscamerasrc ! video/x-raw(memory:NVMM),sensor-id=(int)" + std::to_string(mCameraDeviceIndex)+ ", width=(int)" + std::to_string(mCaptureWidth) + ", height=(int)" +
		std::to_string(mCaptureHeight) + ", format=(string)NV12, framerate=(fraction)" + std::to_string(mFramerate) +
		"/1 ! nvvidconv flip-method=" + std::to_string(mFlipMethod) + " ! video/x-raw, width=(int)" + std::to_string(mDisplayWidth) + ", height=(int)" +
		std::to_string(mDisplayHeight) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

/*****************************************************************
*
* function��CameraCaptureHandler
* ���ã���������ͷ�źŵ�handler
*
*****************************************************************/
CameraCaptureHandler::CameraCaptureHandler(Looper* looper, CameraCapture *cameraCapture) :Handler(looper)
{
	mCameraCapture = cameraCapture;
}

/*****************************************************************
*
* function��handlerMessage
* ���ã���������ͷ�źŵĺ���
*
*****************************************************************/
void CameraCaptureHandler::handlerMessage(Message *message)
{
	timeval startTime;
	timeval endTime;
	gettimeofday(&startTime, nullptr);
	mCameraCapture->doCapture();
	gettimeofday(&endTime, nullptr);
	long delay = getNextFrameDelay(startTime, endTime);
	printf("doCapture end need delay: %ld\n", delay);
	Message* newMessage = Message::obtain(CameraCaptureHandler::CAPTURE_MESSAGE);
	if (delay > 0) 
	{
		sendMessageDelayed(newMessage, delay);
	}
	else
	{
		sendMessage(newMessage);
	}
}

/*****************************************************************
* name:getNextFrameDelay
* function:ʹ��opencvץȡ����ͷ�ź�
* parm:costTime ��ȡ��һ֡����ʱ��
*****************************************************************/
long CameraCaptureHandler::getNextFrameDelay(timeval startTime, timeval endTime)
{
	long temp = (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec)/ 1000;
	printf("doCapture cost time = %ld\n", temp);
	return 1000 / mCameraCapture->getFramerate() - temp;
}
