#pragma once


// #ifdef __cplusplus
// #define __STDC_CONSTANT_MACROS
// #ifdef _STDINT_H
// #undef _STDINT_H
// #endif
// # include "stdint.h"

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libswresample/swresample.h>
#include <libavutil/avstring.h>
};

#include "../../CommonCodes/hav_fifo.h"
#include "ofxUI.h"
#include "ofVbo.h"

//#define INPUT_CAPTURE_SIN 0
#define INPUT_MEDIAFILE 1

typedef struct header_file  
{  
	char chunk_id[4];  
	int chunk_size;  
	char format[4];  
	char subchunk1_id[4];  
	int subchunk1_size;  
	short int audio_format;  
	short int num_channels;  
	int sample_rate;            // sample_rate denotes the sampling rate.  
	int byte_rate;  
	short int block_align;  
	short int bits_per_sample;  
	char subchunk2_id[4];  
	int subchunk2_size;         // subchunk2_size denotes the number of samples.  
} header;  
typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

class FFMPEG
{
public:
	FFMPEG(void);
	~FFMPEG(void);


	void FFMPEG_Init(int64_t, int);
	GLbyte *pPixeldata;
	AVFrame* CapturedFrame;
	GLint iViewPort[4];
	struct SwsContext *img_convert_ctx;
	int outbuf_size;
	uint8_t* outbuf;
	uint8_t* pictYUV_buf;
	AVFrame* pictYUV;
	int frameno;
	int size;
	GLenum lastBuffer;
	
	
	AVPacket encodedVideoPacket;
	AVPacket encodedAudioPacket;
	AVPacket decodedVideoPacket;
	AVPacket deCodedAudioPacket;

	AVFrame* audioFrame;
	int audio_buffer_size;
	uint16_t* audio_sample;

	bool initializeVideoEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeAudioEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeVideoDecoder(enum AVCodecID codecId);
	bool initializeAudioDecoder(enum AVCodecID codecId);
	
	bool initializeMediaFileDecoderEncoder();


	//bool initializeAudioEncoder2(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	void initializeVideoCapture();
	int nextPTS(){
		static int static_pts=0;
		return static_pts++;
	}


	AVFrame* VideoDecode(AVPacket data1);
	float AudioDecode(AVPacket data1, float*);

	AVCodecContext* mVideoEncoderContext;
	AVCodec* mVideoEncoder;
	AVCodecContext* mVideoDecoderContext;
	AVCodec* mVideoDecoder;

	AVCodecContext* mAudioEncoderContext;
	AVCodec* mAudioEncoder;
	AVCodecContext* mAudioDecoderContext;
	AVCodec* mAudioDecoder;
	hav::PacketQueue mVideoEncodeBuffer;
	hav::PacketQueue mAudioEncodeBuffer;
	hav::PacketQueue mVideoBuffer2;
	hav::PacketQueue mAudioBuffer2;

	hav::PacketQueue mAudioTB;
	hav::PacketQueue mVideoTB;

	hav::PacketQueue* getVideoBuffer(){return &mVideoEncodeBuffer;};
	hav::PacketQueue* getAudioBuffer(){return &mAudioEncodeBuffer;};
	
	//Sound 
	AVPacket* SoundCapture();
	AVFrame* SoundCapture2();
	//OpenGL È­¸é Ä¸ÃÄ
	void VideoCapture();
#ifdef INPUT_CAPTURE_SIN 
	unsigned char mRgbBuffer[640*480*3];
#endif

#ifdef INPUT_MEDIAFILE 
	unsigned char mRgbBuffer[1280*720*3];
#endif
	

	bool captureflag1;
	bool captureflag2;

	CRITICAL_SECTION cs;

	int64_t Capture_time;

	AVInputFormat *iformat;
	AVFormatContext *pFormatCtx;
	//const char FileName[255];
	AVDictionary* options;
	AVCodecContext  *aCodecCtx;
	AVCodec* aCodec;

	int videoStream;
	int audioStream;
	AVPacket AFpacket[2];

	int64_t timeoffset;

	struct SwrContext* swr_ctx; 
	AudioParams audio_tgt;
	AudioParams audio_src;
	uint8_t *audio_buf1;
	float* idata;
	typedef struct header_file* header_p;
	unsigned int audio_buf1_size1;

	int apts;
	int tcnt;
	//----------
	char filename[255];

	AVFrame* VideoFrameFormatConvert(AVCodecContext *aCtx, AVFrame *aFrame);

};

