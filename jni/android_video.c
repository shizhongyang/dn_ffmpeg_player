#include "com_dongnaoedu_dnffmpegplayer_VideoPlayer.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);
#define LOGV(FORMAT,...) __android_log_print(ANDROID_LOG_VERBOSE,"jason",FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"jason",FORMAT,##__VA_ARGS__);
double get_rotation(AVStream *st) {
	AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL,
			0);
	double theta = 0;

	if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
		//char *tail;
		//theta = av_strtod(rotate_tag->value, &tail);
		theta = atof(rotate_tag->value);
		// if (*tail)
		// theta = 0;
	}

	theta -= 360 * floor(theta / 360 + 0.9 / 360);

	if (fabs(theta - 90 * round(theta / 90)) > 2)
		LOGV("Odd rotation angle");

	LOGV("get_rotation %f", theta);

	return theta;
}

void frame_rotate_270(AVFrame *src, AVFrame* des) {
	LOGI("旋转开始");
	int n = 0, i = 0, j = 0;
	int hw = src->width >> 1;
	int hh = src->height >> 1;
	int pos = 0;

	for (i = src->width - 1; i >= 0; i--) {
		pos = 0;
		for (j = 0; j < src->height; j++) {
			des->data[0][n++] = src->data[0][pos + i];
			pos += src->width;
		}
	}

	n = 0;
	for (i = hw - 1; i >= 0; i--) {
		pos = 0;
		for (j = 0; j < hh; j++) {
			des->data[1][n] = src->data[1][pos + i];
			des->data[2][n] = src->data[2][pos + i];
			pos += hw;
			n++;
		}
	}

	des->linesize[0] = src->height;
	des->linesize[1] = src->height >> 1;
	des->linesize[2] = src->height >> 1;

	des->width = src->height;
	des->height = src->width;
	des->format = src->format;

	des->pts = src->pts;
	des->pkt_pts = src->pkt_pts;
	des->pkt_dts = src->pkt_dts;

	des->key_frame = src->key_frame;

	LOGI("旋转完成");
}

JNIEXPORT jint JNICALL Java_com_dongnaoedu_dnffmpegplayer_VideoPlayer_render(
		JNIEnv * env, jclass clazz, jstring input_jstr, jobject surface) {
	LOGI("play");

	// sd卡中的视频文件地址,可自行修改或者通过jni传入
	//char * file_name = "/sdcard/Video/00.flv";
	const char* file_name = (*env)->GetStringUTFChars(env, input_jstr, NULL);
	av_register_all();

	AVFormatContext * pFormatCtx = avformat_alloc_context();

	// Open video file
	if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {

		LOGE("Couldn't open file:%s\n", file_name);
		return -1; // Couldn't open file
	}

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("Couldn't find stream information.");
		return -1;
	}

	// Find the first video stream
	int videoStream = -1, i;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
				&& videoStream < 0) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		LOGE("Didn't find a video stream.");
		return -1; // Didn't find a video stream
	}

	double r = get_rotation(pFormatCtx->streams[videoStream]);

	LOGI("旋转的角度是：%f", r)
	/*AVStream *out_stream = avformat_new_stream(pFormatCtx,
			pFormatCtx->streams[videoStream]->codec->codec);

	int ret = av_dict_set(&out_stream->metadata, "rotate", "90", 0); //设置旋转角度
	if (ret >= 0) {
		LOGI("=========yes=====set rotate success!===\n");
	}
	if (avcodec_copy_context(out_stream->codec,
			pFormatCtx->streams[videoStream]->codec) < 0) {
		printf("Failed to copy context from input to output stream codec context\n");
	}
	*/



	/*
	 // Get a pointer to the codec context for the video stream

	 AVCodecContext * pCodecCtx1 = out_stream->codec;
	 if (pCodecCtx1 == NULL) {
	 LOGE("pCodecCtx not found.");
	 return -1; // Codec not found
	 }
	 AVCodec * pCodec1 = avcodec_find_decoder(pCodecCtx1->codec_id);
	 if (pCodec1 == NULL) {
	 LOGE("Codec1 not found.");
	 return -1; // Codec not found
	 }*/

	AVStream *out_stream1 = pFormatCtx->streams[videoStream];
	AVCodecContext * pCodecCtx = out_stream1->codec;
	// Find the decoder for the video stream
	AVCodec * pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		LOGE("Codec not found.");
		return -1; // Codec not found
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.");
		return -1; // Could not open codec
	}

	// 获取native window
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);

	// 获取视频宽高
	int videoWidth = pCodecCtx->width;
	int videoHeight = pCodecCtx->height;

	// 设置native window的buffer大小,可自动拉伸
	ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
			WINDOW_FORMAT_RGBA_8888);
	ANativeWindow_Buffer windowBuffer;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.");
		return -1; // Could not open codec
	}

	// Allocate video frame yuv
	AVFrame * pFrame = av_frame_alloc();

	AVFrame * desFrame = av_frame_alloc();

	// 用于渲染
	AVFrame * pFrameRGBA = av_frame_alloc();
	if (pFrameRGBA == NULL || pFrame == NULL) {
		LOGE("Could not allocate video frame.");
		return -1;
	}

	// Determine required buffer size and allocate buffer
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width,
			pCodecCtx->height, 1);
	uint8_t * buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer,
			AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);

	// 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
	struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
			pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
			pCodecCtx->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

	int frameFinished;
	AVPacket packet;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {

			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			// 并不是decode一次就可解码出一帧
			if (frameFinished) {

				//frame_rotate_270(pFrame,desFrame);

				// lock native window buffer
				ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

				// 格式转换
				sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height,
						pFrameRGBA->data, pFrameRGBA->linesize);

				// 获取stride
				uint8_t * dst = windowBuffer.bits;
				int dstStride = windowBuffer.stride * 4;
				uint8_t * src = (uint8_t*) (pFrameRGBA->data[0]);
				int srcStride = pFrameRGBA->linesize[0];

				// 由于window的stride和帧的stride不同,因此需要逐行复制
				int h;
				for (h = 0; h < videoHeight; h++) {
					memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
				}

				ANativeWindow_unlockAndPost(nativeWindow);
			}

		}
		av_packet_unref(&packet);
	}

	av_free(buffer);
	av_free(pFrameRGBA);

	// Free the YUV frame
	av_free(pFrame);

	// Free the YUV frame
	av_free(desFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	(*env)->ReleaseStringUTFChars(env, input_jstr, file_name);
	return 0;
}
