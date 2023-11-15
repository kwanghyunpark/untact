#pragma once


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
};

#include "../../CommonCodes/hav_fifo.h"
#include "ofxUI.h"
#include "ofVbo.h"


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


class FFMPEG
{
public:
	FFMPEG(void);
	~FFMPEG(void);


	void FFMPEG_Init(int64_t);
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
	AVPacket encoded_packet1;
	AVPacket encoded_packet2;

	AVPacket encodedAudioPacket;
	AVFrame* audioFrame;
	int audio_buffer_size;
	uint16_t* audio_sample;

	bool initializeVideoEncoder1(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeAudioEncoder1(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeVideoEncoder2(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeAudioFileDecoderEncoder();


	//bool initializeAudioEncoder2(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	void initializeVideoCapture();
	
	AVCodecContext* mVideoEncoderContext1;
	AVCodec* mVideoEncoder1;
	AVCodecContext* mVideoEncoderContext2;
	AVCodec* mVideoEncoder2;


	AVCodecContext* mAudioEncoderContext;
	AVCodec* mAudioEncoder;
	hav::PacketQueue mVideoBuffer1;
	hav::PacketQueue mAudioBuffer1;
	hav::PacketQueue mVideoBuffer2;
	hav::PacketQueue mAudioBuffer2;

	hav::PacketQueue mAudioTB;

	hav::PacketQueue* getVideoBuffer1(){return &mVideoBuffer1;};
	hav::PacketQueue* getAudioBuffer1(){return &mAudioBuffer1;};
	hav::PacketQueue* getVideoBuffer2(){return &mVideoBuffer2;};
	hav::PacketQueue* getAudioBuffer2(){return &mAudioBuffer2;};
	//Sound 
	AVFrame* SoundCapture(bool touch1, bool touch2, double dis1, double dis2);
	AVFrame* SoundCapture2();
	//OpenGL È­¸é Ä¸ÃÄ
	void VideoCapture();
	unsigned char mRgbBuffer[640*480*3];

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



	typedef struct header_file* header_p;
};

