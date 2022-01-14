#include "./handlerThread/Handler.h"
#include "./handlerThread/Message.h"
#include "./handlerThread/NThread.h"
#include "./handlerThread/Condition.h"
#include "./handlerThread/Meutex.h"
#include "./rtmp/RtmpManager.h"
#include "./base/BaseHeader.h"

/*******************************************************************
*
*  rtmpClient主函数
*
*********************************************************************/
int main(int argc, char **argv) {

	Looper *mMainLooper = new Looper();
	RtmpManager *rtmpManger = new RtmpManager();
	rtmpManger->startRtmp();
	mMainLooper->loop();
    return 0;
}
