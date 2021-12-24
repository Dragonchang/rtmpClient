#ifndef H_MPP_ENCODER
#define H_MPP_ENCODER
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/hwcontext_drm.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <rockchip/mpp_buffer.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/rk_venc_cfg.h>
#ifdef __cplusplus
}
#endif
typedef struct {
	uint8_t *data;
	uint32_t size;
} SpsHeader;

typedef struct {
	MppPacket packet;
	AVBufferRef *encoder_ref;
} RKMPPPacketContext;
using namespace std;
using namespace cv;
class MppEncoder
{

public:
	MppEncoder();
	virtual ~MppEncoder();

	int init();
	int getSpsInfo(SpsHeader *sps_header);
    /******************************************
	*
	*
	******************************************/
	int rkmpp_encode_frame(const Mat *frame, AVPacket *pkt);
private:
	int mpp_enc_cfg_setup();

private:
	MppCtx mMppCtx;
	MppApi *mMppApi;
	MppCodingType mMppCodingType;
	MppEncCfg mMppEncCfg;
	MppFrameFormat mMppFrameFormat;

	MppBufferGroup buf_grp;
	MppBuffer frm_buf;
	MppBuffer pkt_buf;

	int mWidth;
	int mHeight;
	RK_U32 hor_stride;
	RK_U32 ver_stride;
	RK_S32 rc_mode;

	size_t header_size;
	size_t frame_size;

	RK_S32 bps;
	RK_S32 bps_max;
	RK_S32 bps_min;

	RK_S32 fps_in_flex;
	RK_S32 fps_in_den;
	RK_S32 fps_in_num;
	RK_S32 fps_out_flex;
	RK_S32 fps_out_den;
	RK_S32 fps_out_num;
};
#endif