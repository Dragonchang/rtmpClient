#ifndef __RTMP_MANAGER_H_
#define __RTMP_MANAGER_H_
#include <map>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <string>

#include "../handlerThread/Handler.h"
#include "../handlerThread/Message.h"
#include "../handlerThread/Looper.h"
#include "../handlerThread/NThread.h"
#include "./CameraCapture.h"

using namespace std;
class RtmpManagerHandler;
class CameraCapture;
typedef enum { INIT, RUN, ERROR, STOP } CameraStatus;
class CameraInfo 
{
public:
	int mCameraIndex;
	int mCameraCount;
	CameraStatus mCameraStatus;
	CameraCapture *mCameraCapture;

};
class RtmpManager
{
public:
	

public:
	RtmpManager();
	~RtmpManager();
	void startRtmp();


public:
	/*******************************
	*初始化获取可用的摄像头
	*******************************/
	void freshAviableCamera();

	bool isAllCameraWork();

	/*******************************
	*热插拔发生错误
	*******************************/
	void errorCamera(int cameraIndex);
	void deleteErrorCamera(int* cameraIndex);

	bool createCamera(int cameraIndex);
	bool deleteCamera(int cameraIndex);
	CameraInfo* getCameraInfoByIndex(int cameraIndex);
private:
	int mMaxCameraCount;
	int mCurrentWorkCameraCount;
	map<int, CameraInfo*> mCamerasMap;

	RtmpManagerHandler* mRtmpManagerHandler;
	NThread* mRtmpManagerThread;
};

class RtmpManagerHandler : public Handler {

public:
	static const int START_CAPTURE_MESSAGE = 1;
	static const int CHECK_CAMERA_MESSAGE = 2;
	static const int ERROTR_CAMERA_MESSAGE = 3;

private:
	RtmpManager *mRtmpManager;

public:
	RtmpManagerHandler(Looper* looper, RtmpManager *rtmpManager);

private:
	void handlerMessage(Message *message);
};
#endif