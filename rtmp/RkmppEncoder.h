#ifndef __RKMPP_ENCODER_H_
#define __RKMPP_ENCODER_H_


#include <pthread.h>
#include <rockchip/mpp_buffer.h>
#include <rockchip/rk_mpi.h>
#include <time.h>
#include <unistd.h>


#include <libavcodec/avcodec.h>

#define RKMPP_ENCODER 1
typedef struct {
	MppCtx ctx;
	MppApi *mpi;

	char eos_reached;
} RKMPPEncoder;

typedef struct {
	AVClass *av_class;
	AVBufferRef *encoder_ref;
} RKMPPEncodeContext;

typedef struct {
	MppPacket packet;
	AVBufferRef *encoder_ref;
} RKMPPPacketContext;

class RkmppEncoder
{
public:
	int rkmpp_init_encoder(AVCodecContext *avctx);

	int rkmpp_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
		const AVFrame *frame, int *got_packet);

private:
	MppCodingType rkmpp_get_codingtype(AVCodecContext *avctx);
};

#endif