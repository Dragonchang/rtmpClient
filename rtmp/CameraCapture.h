#ifndef H_CAMERA_CAPTURE
#define H_CAMERA_CAPTURE
#include  "../base/BaseHeader.h"

#include "../handlerThread/Handler.h"
#include "../handlerThread/Message.h"
#include "../handlerThread/Looper.h"
#include "../handlerThread/NThread.h"
#include "./PushRtmp.h"
#include "./RtmpManager.h"

#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/avutil.h>  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>  
#include <libavutil/imgutils.h> 
#ifdef __cplusplus
}
#endif

using namespace std;
using namespace cv;
class CameraCaptureHandler;
class PushRtmp;
class RtmpManager;
class CameraCapture
{
public:
	CameraCapture(RtmpManager* rtmpManager);
	virtual ~CameraCapture();

public:
	void startCameraCapture(int cameraDeviceIndex, int rtmpIndex);
	void doCapture();
	long getFramerate() { return mFramerate; }

private:
	string gstreamerPipeline();
	int openCamera();
	bool isCameraOpen();
	AVFrame* cvmatToAvframe(Mat* image);
	void releaseCamera();
	void releaseAVFrame(AVFrame *avframe);

private:
	CameraCaptureHandler *mCaptureHandler;
	NThread *mCapturethread;
	RtmpManager* mRtmpManager;

	VideoCapture *mVideoCapture;
	SwsContext *m_Vsc;

	//打开摄像参数
	int mCaptureWidth;
	int mCaptureHeight;
	int mDisplayWidth;
	int mDisplayHeight;
	int mFramerate;
	int mFlipMethod;
	int m_vpts;
	//默认使用/dev/video1 usb
	int mCameraDeviceIndex;
        int mRtmpIndex;
	PushRtmp *mPushRtmp;

};

class CameraCaptureHandler : public Handler {

public:
	static const int CAPTURE_MESSAGE = 1;

private:
	CameraCapture *mCameraCapture;

public:
	CameraCaptureHandler(Looper* looper, CameraCapture *cameraCapture);

private:
	void handlerMessage(Message *message);
	long getNextFrameDelay(timeval startTime, timeval endTime);
};
#endif
