#include"MppEncoder.h"
#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))
#define MPP_PACKET_FLAG_INTRA       (0x00000008)

MppEncoder::MppEncoder()
{
	mMppCodingType = MPP_VIDEO_CodingAVC;
	mMppFrameFormat = MPP_FMT_YUV420P;
	mWidth = 640;
	mHeight = 480;
	hor_stride = MPP_ALIGN(mWidth, 16);
	ver_stride = MPP_ALIGN(mHeight, 16);
	rc_mode = MPP_ENC_RC_MODE_CBR;
	bps = 1024*1024;
	frame_size = 460800;
	header_size = 0;
	fps_in_flex = 0;
	fps_in_num = 25;
	fps_in_den = 1;
	fps_out_flex = 0;
	fps_out_den = 1;
	fps_out_num = 25;
}

MppEncoder::~MppEncoder()
{
	if(mMppCtx != NULL) {
		mpp_destroy(mMppCtx);
		mMppCtx = NULL;
	}
	
	if(mMppEncCfg != NULL) {
		mpp_enc_cfg_deinit(mMppEncCfg);
		mMppEncCfg = NULL;
	}

	if (frm_buf != NULL) {
		mpp_buffer_put(frm_buf);
		frm_buf = NULL;
	}
	if (pkt_buf != NULL) {
		mpp_buffer_put(pkt_buf);
		pkt_buf = NULL;
	}
	if (buf_grp != NULL) {
		mpp_buffer_group_put(buf_grp);
		buf_grp = NULL;
	}
}

int MppEncoder::init()
{
	int ret;
	ret = mpp_buffer_group_get_internal(&buf_grp, MPP_BUFFER_TYPE_DRM);
	if (ret) {
		printf("failed to get mpp buffer group ret %d\n", ret);
		goto MPP_INIT_OUT;
	}
	ret = mpp_buffer_get(buf_grp, &frm_buf, frame_size + header_size);
	if (ret) {
		printf("failed to get buffer for input frame ret %d\n", ret);
		goto MPP_INIT_OUT;
	}

	ret = mpp_buffer_get(buf_grp, &pkt_buf, frame_size);
	if (ret) {
		printf("failed to get buffer for output packet ret %d\n", ret);
		goto MPP_INIT_OUT;
	}

	ret = mpp_create(&mMppCtx, &mMppApi);
	if (ret) {
		printf("mpp_create failed ret %d\n", ret);
		goto MPP_INIT_OUT;
	}
	ret = mpp_init(mMppCtx, MPP_CTX_ENC, mMppCodingType);
	if (ret)
	{
		printf("mpp_init failed ret %d\n", ret);
		goto MPP_INIT_OUT;
	}
	ret = mpp_enc_cfg_init(&mMppEncCfg);
	if (ret) {
		printf("mpp_enc_cfg_init failed ret %d\n", ret);
		goto MPP_INIT_OUT;
	}
	ret = mpp_enc_cfg_setup();
	if (ret)
	{
		printf("mpp_enc_cfg_setup failed ret %d\n", ret);
		goto MPP_INIT_OUT;
	}
	return 0;

MPP_INIT_OUT:

	if (mMppCtx)
	{
		mpp_destroy(mMppCtx);
		mMppCtx = NULL;
	}
	printf("init mpp failed!\n");
	return -1;
}

int MppEncoder::getSpsInfo(SpsHeader * sps_header)
{
	MPP_RET ret = MPP_OK;
	if (mMppCodingType == MPP_VIDEO_CodingAVC)
	{
		MppPacket packet = NULL;
		mpp_packet_init_with_buffer(&packet, pkt_buf);
		mpp_packet_set_length(packet, 0);
		ret = mMppApi->control(mMppCtx, MPP_ENC_GET_HDR_SYNC, packet);
		if (ret) {
			printf("mpi control enc get extra info failed\n");
			mpp_packet_deinit(&packet);
			return ret;
		}
		else {
			/* get and write sps/pps for H.264 */

			void *ptr = mpp_packet_get_pos(packet);
			size_t len = mpp_packet_get_length(packet);
			sps_header->data = (uint8_t*)malloc(len);
			sps_header->size = len;
			memcpy(sps_header->data, ptr, len);
		}
		mpp_packet_deinit(&packet);
	}
	return ret;
}

int MppEncoder::rkmpp_encode_frame(const Mat* avframe, AVPacket* pkt)
{
	MPP_RET ret = MPP_OK;
	if (avframe == NULL) {
		printf("rkmpp_encode_frame failed with null frame\n");
		return 1;
	}
	MppFrame frame = NULL;
	MppPacket packet = NULL;
	MppMeta meta = NULL;
	MppBufferInfo info;
	MppBuffer buffer = NULL;

	void *buf = mpp_buffer_get_ptr(frm_buf);
	size_t buff_size = mpp_buffer_get_size(frm_buf);
	int rows = avframe->rows;
	int cols = avframe->cols;
	int num_el = rows * cols;
	int data_size = num_el * avframe->elemSize();
	memcpy(buf, avframe->data, data_size);
	ret = mpp_frame_init(&frame);
	if (ret != MPP_OK) {
		printf("Failed init mpp frame on encoder (code = %d)\n", ret);
		return 1;
	}

	mpp_frame_set_width(frame, mWidth);
	mpp_frame_set_height(frame, mHeight);
	mpp_frame_set_hor_stride(frame, hor_stride);
	mpp_frame_set_ver_stride(frame, ver_stride);
	mpp_frame_set_fmt(frame, mMppFrameFormat);
	mpp_frame_set_buffer(frame, frm_buf);

	meta = mpp_frame_get_meta(frame);
	mpp_packet_init_with_buffer(&packet, pkt_buf);
	/* NOTE: It is important to clear output packet length!! */
	mpp_packet_set_length(packet, 0);
	mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
	ret = mMppApi->encode_put_frame(mMppCtx, frame);
	if (ret) {
		printf("mpp encode put frame failed\n");
		mpp_frame_deinit(&frame);
		mpp_packet_deinit(&packet);
		return 1;
	}
	mpp_frame_deinit(&frame);

	do {
		ret = mMppApi->encode_get_packet(mMppCtx, &packet);
		if (ret) {
			printf("mpp encode get packet failed\n");
			break;
		}

		if (packet) {
			RK_U32 flag;
			// TODO: outside need fd from mppbuffer?
			pkt->data = (uint8_t *)mpp_packet_get_data(packet);
			pkt->size = mpp_packet_get_length(packet);
			pkt->pts = mpp_packet_get_pts(packet);
			pkt->dts = mpp_packet_get_dts(packet);
			if (pkt->pts <= 0)
				pkt->pts = pkt->dts;
			if (pkt->dts <= 0)
				pkt->dts = pkt->pts;
			flag = mpp_packet_get_flag(packet);
			if (flag & MPP_PACKET_FLAG_INTRA)
				pkt->flags |= AV_PKT_FLAG_KEY;
		}
	} while (0);
	mpp_packet_deinit(&packet);
	return ret;
}

int MppEncoder::mpp_enc_cfg_setup()
{
	MPP_RET ret = MPP_OK;
	MppPollType timeout = MPP_POLL_BLOCK;
	ret = mMppApi->control(mMppCtx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
	if (MPP_OK != ret) {
		printf("mpi control set output timeout %d ret %d\n", timeout, ret);
		return ret;
	}
	mpp_enc_cfg_set_s32(mMppEncCfg, "prep:width", mWidth);
	mpp_enc_cfg_set_s32(mMppEncCfg, "prep:height", mHeight);
	mpp_enc_cfg_set_s32(mMppEncCfg, "prep:hor_stride", hor_stride);
	mpp_enc_cfg_set_s32(mMppEncCfg, "prep:ver_stride", ver_stride);
	mpp_enc_cfg_set_s32(mMppEncCfg, "prep:format", mMppFrameFormat);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:mode", rc_mode);

	/* fix input / output frame rate */
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_in_flex", fps_in_flex);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_in_num", fps_in_num);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_in_denorm", fps_in_den);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_out_flex", fps_out_flex);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_out_num", fps_out_num);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:fps_out_denorm", fps_out_den);
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:gop", fps_out_num * 2);

	/* drop frame or not when bitrate overflow */
	mpp_enc_cfg_set_u32(mMppEncCfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
	mpp_enc_cfg_set_u32(mMppEncCfg, "rc:drop_thd", 20);        /* 20% of max bps */
	mpp_enc_cfg_set_u32(mMppEncCfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

	/* setup bitrate for different rc_mode */
	mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_target", bps);


	switch (rc_mode) {
	case MPP_ENC_RC_MODE_FIXQP: {
		/* do not setup bitrate on FIXQP mode */
	} break;
	case MPP_ENC_RC_MODE_CBR: {
		/* CBR mode has narrow bound */
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_max", bps_max ? bps_max : bps * 17 / 16);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_min", bps_min ? bps_min : bps * 15 / 16);
	} break;
	case MPP_ENC_RC_MODE_VBR:
	case MPP_ENC_RC_MODE_AVBR: {
		/* VBR mode has wide bound */
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_max", bps_max ? bps_max : bps * 17 / 16);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_min", bps_min ? bps_min : bps * 1 / 16);
	} break;
	default: {
		/* default use CBR mode */
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_max", bps_max ? bps_max : bps * 17 / 16);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:bps_min", bps_min ? bps_min : bps * 15 / 16);
	} break;
	}

	/* setup qp for different codec and rc_mode */
	switch (mMppCodingType) {
	case MPP_VIDEO_CodingAVC:
	case MPP_VIDEO_CodingHEVC: {
		switch (rc_mode) {
		case MPP_ENC_RC_MODE_FIXQP: {
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_init", 20);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max", 20);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min", 20);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max_i", 20);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min_i", 20);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_ip", 2);
		} break;
		case MPP_ENC_RC_MODE_CBR:
		case MPP_ENC_RC_MODE_VBR:
		case MPP_ENC_RC_MODE_AVBR: {
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_init", 26);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max", 51);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min", 10);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max_i", 51);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min_i", 10);
			mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_ip", 2);
		} break;
		default: {
			printf("unsupport encoder rc mode %d\n", rc_mode);
		} break;
		}
	} break;
	case MPP_VIDEO_CodingVP8: {
		/* vp8 only setup base qp range */
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_init", 40);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max", 127);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min", 0);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_max_i", 127);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_min_i", 0);
		mpp_enc_cfg_set_s32(mMppEncCfg, "rc:qp_ip", 6);
	} break;
	case MPP_VIDEO_CodingMJPEG: {
		/* jpeg use special codec config to control qtable */
		mpp_enc_cfg_set_s32(mMppEncCfg, "jpeg:q_factor", 80);
		mpp_enc_cfg_set_s32(mMppEncCfg, "jpeg:qf_max", 99);
		mpp_enc_cfg_set_s32(mMppEncCfg, "jpeg:qf_min", 1);
	} break;
	default: {
	} break;
	}

	/* setup codec  */
	mpp_enc_cfg_set_s32(mMppEncCfg, "codec:type", mMppCodingType);
	switch (mMppCodingType) {
	case MPP_VIDEO_CodingAVC: {
		/*
		 * H.264 profile_idc parameter
		 * 66  - Baseline profile
		 * 77  - Main profile
		 * 100 - High profile
		 */
		mpp_enc_cfg_set_s32(mMppEncCfg, "h264:profile", 100);
		/*
		 * H.264 level_idc parameter
		 * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
		 * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
		 * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
		 * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
		 * 50 / 51 / 52         - 4K@30fps
		 */
		mpp_enc_cfg_set_s32(mMppEncCfg, "h264:level", 40);
		mpp_enc_cfg_set_s32(mMppEncCfg, "h264:cabac_en", 1);
		mpp_enc_cfg_set_s32(mMppEncCfg, "h264:cabac_idc", 0);
		mpp_enc_cfg_set_s32(mMppEncCfg, "h264:trans8x8", 1);
	} break;
	case MPP_VIDEO_CodingHEVC:
	case MPP_VIDEO_CodingMJPEG:
	case MPP_VIDEO_CodingVP8: {
	} break;
	default: {
		printf("unsupport encoder coding type %d\n", mMppCodingType);
	} break;
	}

	//RK_U32 split_mode = 0;
	//RK_U32 split_arg = 0;

	//mpp_enc_cfg_get_u32("split_mode", &split_mode, MPP_ENC_SPLIT_NONE);
	//mpp_enc_cfg_get_u32("split_arg", &split_arg, 0);

	//if (p->split_mode) {
		//printf("%p split_mode %d split_arg %d\n", mMppCtx, split_mode, split_arg);
		//mpp_enc_cfg_set_s32(mMppEncCfg, "split:mode", split_mode);
		//mpp_enc_cfg_set_s32(mMppEncCfg, "split:arg", split_arg);
	//}

	ret = mMppApi->control(mMppCtx, MPP_ENC_SET_CFG, mMppEncCfg);
	if (ret) {
		printf("mpi control enc set cfg failed ret %d\n", ret);
	}
	return ret;
}
