#pragma once




#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include "stdint.h"

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

#endif

#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include "../../CommonCodes/hav_fifo.h"

/*
typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;
*/

enum eFFMpegMediaClassOwnerType{
	MediaStreamServer,
	MediaStreamClient,
	NotYetDesignated
};

//FFMPEGMEDIA_API void packet_queue_init(PacketQueue *q);
//FFMPEGMEDIA_API int packet_queue_put(PacketQueue *q, AVPacket *pkt);
//FFMPEGMEDIA_API int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);
//FFMPEGMEDIA_API void packet_queue_flush(PacketQueue *q);

/*
class FFMPEGMEDIA_API ICoDecodable
{
public:
	virtual int capture() = 0;
	virtual int encode() = 0;
	virtual int decode() = 0;
	virtual int render() = 0;
};

class FFMPEGMEDIA_API CMediaStream : public ICoDecodable
{
public:
	CMediaStream();
	~CMediaStream();

protected:
	int mIdxInputStream;

	AVFormatContext* mFormatContext;

	AVCodecContext* mCapturerCodecContext;
	AVCodec* mCapturerCodec;

	AVCodecContext* mEncoderContext;
	AVCodec* mEncoder;

	AVCodecContext* mDecoderContext;
	AVCodec* mDecoder;

	SwsContext *mSwsContext;
	hav::PacketQueue mBuffer;
};
*/


class  CMediaCodec
{
private:
	int64_t mNTPOffset;
	int mIdxInputVideoStream;
	int mIdxInputAudioStream;

	eFFMpegMediaClassOwnerType mOwnerType;
	//PacketQueue gVideoQueue;
	AVFormatContext* mFormatContext;

	// for video
	AVCodecContext* mVideoCapturerCodecContext;
	AVCodec* mVideoCapturerCodec;
	AVCodecContext* mVideoEncoderContext;
	AVCodecContext* mVideoDecoderContext;
	AVCodec* mVideoEncoder;
	AVCodec* mVideoDecoder;
	
	SwsContext *mSwsContext;

	// for audio
	AVCodecContext* mAudioCapturerCodecContext;
	AVCodec* mAudioCapturerCodec;

	AVCodecContext* mAudioEncoderContext;
	AVCodecContext* mAudioDecoderContext;
	AVCodec* mAudioEncoder;
	AVCodec* mAudioDecoder;
   
	hav::PacketQueue mVideoBuffer;
	hav::PacketQueue mAudioBuffer;

public:
	CMediaCodec(void);
	~CMediaCodec(void);

	hav::PacketQueue* getVideoBuffer(){return &mVideoBuffer;};
	hav::PacketQueue* getAudioBuffer(){return &mAudioBuffer;};

	AVFormatContext* getFormatContext(){return mFormatContext;};
	// for video
	AVCodecContext* getVideoEncoderContext(){return mVideoEncoderContext;};
	AVCodecContext* getVideoDecoderContext(){return mVideoDecoderContext;};
	AVCodecContext* getVideoCapturerContext(){return mVideoCapturerCodecContext;};
	
	// for audio
	AVCodecContext* getAudioEncoderContext(){return mAudioEncoderContext;};
	AVCodecContext* getAudioDecoderContext(){return mAudioDecoderContext;};
	AVCodecContext* getAudioCapturerContext(){return mAudioCapturerCodecContext;};
	
	// Initialization function 
	bool initialize(eFFMpegMediaClassOwnerType type, const char* deviceName, enum AVCodecID codecId=AV_CODEC_ID_MPEG1VIDEO, int width=640, int height=480, int bitRate=500000, int gop_size=30, int64_t timeOffset = 0);
	// Capturer 
	int captureMediaFrame();

	AVFrame* captureVideoFrame();
	AVFrame* captureAudioFrame();
	// Renderer
	void renderVideoFrame(AVFrame* aFrame, SDL_Surface* screen, AVCodecContext* pCodecCtx, int location_x = 0);
	void renderAudioFrame(AVFrame* aFrame, SDL_Surface* screen, AVCodecContext* pCodecCtx, int location_x = 0);
	// Encoder 
	int encodeVideoFrameToPacket(AVFrame* aFrame, AVPacket& encoded_packet);
	int encodeAudioFrameToPacket(AVFrame* aFrame, AVPacket& encoded_packet);
	// Decoder 
	AVFrame* decodeVideoPacketToFrame(AVPacket packet);
	AVFrame* decodeAudioPacketToFrame(AVPacket packet);

	void startMediaCapture();	
	void stopMediaCapture();

	

private:
	bool initializeVideoCapturer();
	bool initializeVideoEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeVideoDecoder(enum AVCodecID codecId);
	AVFrame* createFrameConvertedToYUV420(AVCodecContext *aCtx, AVFrame *aFrame);

	bool initializeAudioCapturer();
	bool initializeAudioEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size);
	bool initializeAudioDecoder(enum AVCodecID codecId);

};

//#endif