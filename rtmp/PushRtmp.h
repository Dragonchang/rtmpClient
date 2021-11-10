#ifndef __PUSH_RTMP_H_
#define __PUSH_RTMP_H_
#include  "../base/BaseHeader.h"

#include "../handlerThread/Handler.h"
#include "../handlerThread/Message.h"
#include "../handlerThread/Looper.h"
#include "../handlerThread/NThread.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif
using namespace std;

class PushRtmpHandler;
class PushRtmp
{
public:
    PushRtmp(int width, int height, int fps, int cameraDeviceIndex);
	virtual ~PushRtmp();
	bool pushRtmp(AVFrame *frame);
	int getFps() { return m_fps; }
	int getFrameCount(){ return m_framecnt;}
	void reduceFrameCount() { --m_framecnt;}

	int getVpts() { return m_vpts; }
	void setVptes(int vpts) { m_vpts = vpts; }

	int getRtmpPushFailedTimes() { return ++m_localRtmpFailedTimes;}
	void setRtmpPushFailedTimes(int times) { m_localRtmpFailedTimes = times; }
	bool getRtmpStatus() { return m_rtmpStatus;}
	void setRtmpStatus(bool status) { m_rtmpStatus = status; }
	AVStream *getOutstream() { return m_out_stream; }
	AVFormatContext * getAVFormatContext() { return m_octx; }
	AVCodecContext *getAVCodecContext() { return m_vc; }
	bool initRtmp();
	void releaseAVFrame(AVFrame *avframe);
private:
	//srs推流地址
	string mOutUrl;
	//推流的camrea index
	int mCameraDeviceIndex;
	bool m_rtmpStatus;
	int m_width;
	int m_height;
	int m_fps;

	int m_framecnt;
	int m_localRtmpFailedTimes;
	int m_vpts;

	AVStream *m_out_stream;
	AVFormatContext * m_octx;
	AVCodecContext *m_vc;

	PushRtmpHandler *mPushRtmpHandler;
	NThread *mPushRtmpThread;
};

class PushRtmpHandler : public Handler {

public:
	static const int CAPTURE_MESSAGE = 1;

private:
	PushRtmp *mPushRtmp;

public:
	PushRtmpHandler(Looper* looper, PushRtmp *pushRtmp);

private:
	void handlerMessage(Message *message);
};


#endif
