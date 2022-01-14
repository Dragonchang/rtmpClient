#include "RtmpManager.h"
RtmpManager::RtmpManager()
{
	//default config with two camera
	mMaxCameraCount = 2;
	mCurrentWorkCameraCount = 0;
	mRtmpManagerThread = new NThread();
	mRtmpManagerHandler = new RtmpManagerHandler(mRtmpManagerThread->getLooper(), this);
}

RtmpManager::~RtmpManager()
{	
	//stop capture thread
	if (mRtmpManagerThread != NULL) {
		delete mRtmpManagerThread;
		mRtmpManagerThread = NULL;
	}
	if (mRtmpManagerHandler != NULL) {
		mRtmpManagerHandler->removeAndDeleteAllMessage();
		delete mRtmpManagerHandler;
		mRtmpManagerHandler = NULL;
	}

	map<int, CameraInfo*>::iterator iter;
	for (iter = mCamerasMap.begin(); iter != mCamerasMap.end(); iter++) {
		CameraInfo* info = iter->second;
		if (info != NULL) {
			if (info->mCameraCapture != NULL) {
				delete info->mCameraCapture;
				info->mCameraCapture = NULL;
			}
		}
	}
}

void RtmpManager::startRtmp()
{
	printf("startRtmp\n");
	Message* message = Message::obtain(RtmpManagerHandler::START_CAPTURE_MESSAGE);
	mRtmpManagerHandler->sendMessage(message);
}

void RtmpManager::freshAviableCamera()
{
	vector<int> indexVector;
	DIR *pDir;
	struct dirent* ptr;
	if (!(pDir = opendir("/dev"))) {
		printf("freshAviableCamera failed with /dev open faild\n");
		return;
	}
	while ((ptr = readdir(pDir)) != NULL) {
		if (strstr(ptr->d_name, "video") != NULL) {
			char *fileName = ptr->d_name;
			int length = strlen(fileName);
			if (length >= 6 && fileName[5] >= '0' && fileName[5] <= '9') {
				char* indexStr = new char[length - 5];
				memcpy(indexStr, fileName+5, length - 5);
				indexVector.push_back(atoi(indexStr));
				delete indexStr;
			}
		}
	}
	closedir(pDir);
	sort(indexVector.begin(), indexVector.end());
	vector<int>::iterator it;
	int mod = 1;
	for (it = indexVector.begin(); it != indexVector.end(); it++) {
		printf("getAviableCamera indexVector with %d\n", *it);
		if (mod % 2 != 0) {
			createCamera(*it);
		}
		mod++;
	}
	if (!isAllCameraWork()) {
		printf("getAviableCamera not all camera work, check it\n");
		Message* message = Message::obtain(RtmpManagerHandler::CHECK_CAMERA_MESSAGE);
		//两秒之后检查是否有可用的摄像头
		mRtmpManagerHandler->sendMessageDelayed(message, 2000);
	}
}

bool RtmpManager::isAllCameraWork()
{
	printf("isAllCameraWork %d\n", mCamerasMap.size());
	if (mCamerasMap.size() >= 2) {
		map<int, CameraInfo*>::iterator iter;
		for (iter = mCamerasMap.begin(); iter != mCamerasMap.end(); iter++) {
			CameraInfo* info = iter->second;
			if (info != NULL ) {
				if (info->mCameraStatus != RUN) {
					return false;
				}
			}
		}
		return true;
	}
	else {
		return false;
	}
}

void RtmpManager::errorCamera(int cameraIndex)
{
	printf("errorCamera index:%d\n", cameraIndex);
	/*
	int* tempCameraIndex = new int();
	*tempCameraIndex = cameraIndex;
	Message* message = Message::obtain(RtmpManagerHandler::ERROTR_CAMERA_MESSAGE, tempCameraIndex);
	mRtmpManagerHandler->sendMessage(message);
	*/
	exit(0);
}

void RtmpManager::deleteErrorCamera(int* cameraIndex)
{
	CameraInfo* info = getCameraInfoByIndex(*cameraIndex);
	if (info != NULL && info->mCameraStatus == RUN) {
		info->mCameraStatus = ERROR;
		if (info->mCameraCapture != NULL) {
			printf("deleteErrorCamera index:%d\n", *cameraIndex);
			delete info->mCameraCapture;
			info->mCameraCapture = NULL;
		}
	}
	delete cameraIndex;
	if (!isAllCameraWork()) {
		printf("deleteErrorCamera not all camera work, check it\n");
		Message* message = Message::obtain(RtmpManagerHandler::CHECK_CAMERA_MESSAGE);
		//两秒之后检查是否有可用的摄像头
		mRtmpManagerHandler->sendMessageDelayed(message, 2000);
	}
}

bool RtmpManager::createCamera(int cameraIndex)
{
	printf("createCamera cameraIndex with %d\n", cameraIndex);
	//first find a avaliable camera info
	map<int, CameraInfo*>::iterator iter;
	for (iter = mCamerasMap.begin(); iter != mCamerasMap.end(); iter++) {
		CameraInfo* info = iter->second;
		if (info != NULL && info->mCameraStatus != RUN) {
			printf("createCamera find a avaliable with %d\n", info->mCameraIndex);
			info->mCameraIndex = cameraIndex;
			info->mCameraStatus = RUN;
			info->mCameraCapture = new CameraCapture(this);
			info->mCameraCapture->startCameraCapture(cameraIndex, info->mCameraCount);
			return true;
		}
	}
	
	CameraInfo* info = getCameraInfoByIndex(cameraIndex);
	if (info == NULL) {
		mCurrentWorkCameraCount++;
		info = new CameraInfo();
		info->mCameraIndex = cameraIndex;
		info->mCameraCount = mCurrentWorkCameraCount;
		info->mCameraStatus = RUN;
		info->mCameraCapture = new CameraCapture(this);
		info->mCameraCapture->startCameraCapture(cameraIndex, mCurrentWorkCameraCount);
		mCamerasMap.insert({ cameraIndex, info });
		return true;
	}
	else
	{
		printf("createCamera with exist camera Index with %d\n", cameraIndex);
	}
	return false;
}

bool RtmpManager::deleteCamera(int cameraIndex)
{
	return false;
}

CameraInfo * RtmpManager::getCameraInfoByIndex(int cameraIndex)
{
	map<int, CameraInfo*>::iterator iter;
	iter = mCamerasMap.find(cameraIndex);
	if (iter != mCamerasMap.end()) {
		return iter->second;
	}
	else {
		return NULL;
	}
}

RtmpManagerHandler::RtmpManagerHandler(Looper * looper, RtmpManager * rtmpManager) :Handler(looper)
{
	mRtmpManager = rtmpManager;
}

void RtmpManagerHandler::handlerMessage(Message * message)
{
	switch (message->what) {
		case START_CAPTURE_MESSAGE:
			mRtmpManager->freshAviableCamera();
		break;
		case CHECK_CAMERA_MESSAGE:
			mRtmpManager->freshAviableCamera();
		break;
		case ERROTR_CAMERA_MESSAGE:
			int* cameraIndex = (int*)message->mObj;
			mRtmpManager->deleteErrorCamera(cameraIndex);
		break;
	}
}
