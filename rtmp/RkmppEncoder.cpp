#include "RkmppEncoder.h"
int RkmppEncoder::rkmpp_init_encoder(AVCodecContext *avctx)
{
	int ret;
	MppCodingType codectype;
	RKMPPEncodeContext *rk_context;
	RKMPPEncoder *encoder;
	MppEncPrepCfg prep_cfg;
	MppEncRcCfg rc_cfg;
	MppEncCodecCfg codec_cfg;
	//RK_S64 paramS64;
	MppEncSeiMode sei_mode;
	MppPacket packet = NULL;
/*	rk_context = avctx->priv_data;
	rk_context->encoder_ref = NULL;
	codectype = rkmpp_get_codingtype(avctx);
	if (codectype == MPP_VIDEO_CodingUnused) {
		//av_log(avctx, AV_LOG_ERROR, "Unsupport codec type (%d).\n", avctx->codec_id);
		ret = AVERROR_UNKNOWN;
	}
	*/
	return 0;
}

int RkmppEncoder::rkmpp_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
	const AVFrame *frame, int *got_packet)
{
	int ret;
	return 0;
}

MppCodingType RkmppEncoder::rkmpp_get_codingtype(AVCodecContext *avctx)
{
	switch (avctx->codec_id) {
	case AV_CODEC_ID_H264:          return MPP_VIDEO_CodingAVC;
	default:                        return MPP_VIDEO_CodingUnused;
	}
}