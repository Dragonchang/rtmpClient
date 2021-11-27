#include "./handlerThread/Handler.h"
#include "./handlerThread/Message.h"
#include "./handlerThread/NThread.h"
#include "./handlerThread/Condition.h"
#include "./handlerThread/Meutex.h"
#include "./rtmp/CameraCapture.h"
#include "./base/BaseHeader.h"

/*******************************************************************
*
*  rtmpClient主函数
*
*********************************************************************/
int main(int argc, char **argv) {

	Looper *mMainLooper = new Looper();
	CameraCapture *mCameraCapture1 = new CameraCapture();
	mCameraCapture1->startCameraCapture(0);

	CameraCapture *mCameraCapture2 = new CameraCapture();
	mCameraCapture1->startCameraCapture(2);

	mMainLooper->loop();
    return 0;
}
