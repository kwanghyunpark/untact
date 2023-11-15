/*
#include "ofMain.h"
#include "testApp.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main( ){

    ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1024,768, OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp( new testApp());

}
*/
#define ADDR_SERVER "114.70.63.186"
//#define ADDR_SERVER "192.168.0.11"

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
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
};

#endif

#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
#include "../../apps/ChicMediaNetwork/FFMpegMediaDll/MediaCodec.h"
#include "ofxThreadChicNetClient.h"
#include "TaskForSyncNetworkTime.h"

hav::PacketQueue gVideoQueue;
hav::PacketQueue gAudioQueue;

SDL_Surface* gScreen;


#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define SDL_AUDIO_BUFFER_SIZE 441
#define MAX_AUDIO_FRAME_SIZE 192000


#define CHANNEL_BASE 1000
#define CHANNEL_VIDEO  (CHANNEL_BASE+0)
#define CHANNEL_AUDIO  (CHANNEL_BASE+1)
#define CHANNEL_HAPTIC (CHANNEL_BASE+2)
#define CHANNEL_RTCP (CHANNEL_BASE+3)

struct encodeThreadData{
	CMediaCodec* codec;
	ofxThreadChicNetClient* threadClient;
};

int64_t gNtpOffset = 0;

bool __stdcall onReceive(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);

CMediaCodec mediaDecoder;

static int decode_thread(void* data) 
{
	CMediaCodec* decoder = (CMediaCodec*)data;

	for(;;){

		//Sleep(5);
		AVPacket packet;
		if(gVideoQueue.nb_packets > 0){
			int64_t  time_stamp;
			if(hav::packet_queue_get(&gVideoQueue, &packet, 1, &time_stamp) > 0) {
							
				if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
					
					AVFrame* decodedFrame = decoder->decodeVideoPacketToFrame(packet);
					if(decodedFrame != NULL){
						decoder->renderVideoFrame(decodedFrame, gScreen, decoder->getVideoDecoderContext());
						av_free(decodedFrame);	
					}
				}
				
				//av_free_packet(&packet);
			}
		}
	}
	return 0;
}

bool __stdcall onReceive(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	static int cnt = 0;

	if(nBufLen <= 0)return false;

	unsigned char* packetData = (unsigned char*)pBuf;
	
	if(nChannel == CHANNEL_VIDEO){

		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		// --------------------------------
		// Read data hear
		// --------------------------------
		char head = avio_r8(avioContext);
		if(head == '#'){
			int64_t time_stamp = avio_rb64(avioContext);
			// -------------------------------
			//printf("%c : %lld (%d) \n", head, time_stamp, nBufLen);
			AVPacket encoded_packet;
			av_init_packet(&encoded_packet);
			encoded_packet.data = (uint8_t*)packetData + (sizeof(char) + sizeof(int64_t));
			encoded_packet.size = nBufLen - (sizeof(char) + sizeof(int64_t));
			hav::packet_queue_put(&gVideoQueue, &encoded_packet, time_stamp);
		}
		av_free(avioContext);
		return true;
	}else if(nChannel == CHANNEL_AUDIO){
		
		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		// --------------------------------
		// Read data hear
		// --------------------------------
		char head = avio_r8(avioContext);
		if(head == '#'){
			int64_t time_stamp = avio_rb64(avioContext);
		// -------------------------------
		//	printf("%c : %lld (%d) \n", head, time_stamp, nBufLen);
			AVPacket encoded_packet;
			av_init_packet(&encoded_packet);
			encoded_packet.data = (uint8_t*)packetData + (sizeof(char) + sizeof(int64_t));
			encoded_packet.size = nBufLen - (sizeof(char) + sizeof(int64_t));
			hav::packet_queue_put(&gAudioQueue, &encoded_packet, time_stamp);
			
		}
		av_free(avioContext);
		return true;
	}else if(nChannel == CHANNEL_HAPTIC){
		//printf("received %d-packet\n", nBufLen);
		for(int i=0; i< nBufLen; i++){
			//printf("%c", packetData[i]);
		}
		return true;
	}

	return false;
}


int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {


	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;
	static AVFrame frame;

	int len1, data_size = 0;
	int64_t time_stamp;
	

	if(gAudioQueue.nb_packets < 20){
		return -1;
	}

		if(hav::packet_queue_get(&gAudioQueue, &pkt, 1, &time_stamp) < 0) {
			return -1;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
		printf("%d \n", gAudioQueue.nb_packets);
		

		while(audio_pkt_size > 0) {

			int got_frame = 0;
			len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
			if(len1 < 0) {
				/* if error, skip frame */
				audio_pkt_size = 0;
				break;
			}
			//audio_pkt_data += len1;
			audio_pkt_size -= len1;
			if (got_frame){
				data_size = av_samples_get_buffer_size(	NULL, aCodecCtx->channels,frame.nb_samples,aCodecCtx->sample_fmt,1);
				memcpy(audio_buf, frame.data[0], data_size);
			}
			if(data_size <= 0) {
				/* No data yet, get more frames */
				continue;
			}
			/* We have data, return it and come back for more later */
			return data_size;
		}
		//if(pkt.data)
		//av_free_packet(&pkt);

		/*
		int64_t ctime = av_gettime();
		double dt = 0.02;

		//av_init_packet(&pkt);
		if((double)(ctime - gAudioQueue.time_stamp)*0.000001 > dt+0.0001){
			if(hav::packet_queue_get(&gAudioQueue, &pkt, 1, &time_stamp) < 0) {
				return -1;
			}
			if((double)(ctime - time_stamp)*0.000001 < dt-0.0001){
				audio_pkt_data = pkt.data;
				audio_pkt_size = pkt.size;
			}
			printf("%lld - %lld = %lld \n",ctime , time_stamp, ctime-time_stamp);
			//printf("%lld\n",gAudioQueue.time_stamp);
		}
		*/

}


int audio_decode_frame3(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

	AVPacket pkt;
	AVPacket* audio_pkt = NULL;
	static int audio_pkt_size = 0;
	static AVFrame frame;

	int len1, data_size = -1;
	int64_t time_stamp;
	int tot_data_size = -1;
	
	int64_t time_lastframe = gAudioQueue.time_stamp;
	int64_t time_current = av_gettime() + gNtpOffset;
	double diff = (double)(time_current - time_lastframe)*0.000001;
	printf("dt = %.3f s\n", diff);
	
	if(hav::packet_queue_get(&gAudioQueue, &pkt, 1, &time_stamp) < 0) {
		return -1;
	}
	
	//printf("dt = %.3f s : current: %lld , estimated: %lld \n",  (double)(currentTime - time_stamp)*0.000001, currentTime, time_stamp);
	audio_pkt_size = pkt.size;

	while(audio_pkt_size > 0) {

		int got_frame = 0;
		len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
		if(len1 < 0) {
			//if error, skip frame 
			audio_pkt_size = 0;
			break;
		}

		audio_pkt_size -= len1;

		if (!got_frame){
			continue;
		}
		data_size = av_samples_get_buffer_size(	NULL, av_frame_get_channels(&frame), frame.nb_samples, (AVSampleFormat)frame.format,1);
		if(data_size <= 0) {
			// No data yet, get more frames 
			continue;
		}
		memcpy(audio_buf, frame.data[0], data_size);
		// We have data, return it and come back for more later 
		return data_size;
	}

	//av_free_packet(&pkt);
	//av_free(&pkt);

	return -1;
}

static int cnt =0;
void audio_callback(void *userdata, Uint8 *stream, int len) {

	AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
	int len1, audio_size;
	static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	while(len > 0) {
		if(audio_buf_index >= audio_buf_size) {
			// We have already sent all our data; get more 
			audio_size = audio_decode_frame3(aCodecCtx, audio_buf, audio_buf_size);
			if(audio_size < 0) {
				// If error, output silence 
				audio_buf_size = 1024; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			} else {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;
		}
		len1 = audio_buf_size - audio_buf_index;
		if(len1 > len)
			len1 = len;

		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);

		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}

}


int64_t getCurrentSystemTimeMicros()
{
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time

	// get the high resolution counter's accuracy
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);
	return (1000000*tick.QuadPart / ticksPerSecond.QuadPart);
}

int main(int argc, char* argv[])
{
	CNTPSynchronizer ntpTimer;
	ntpTimer.runNTPSynchronization();
	gNtpOffset = ntpTimer.getOffset();

	ofxThreadChicNetClient threadChicNetClient;

	std::map<int, std::string> device;
	//device[0] = "video";
	
	if(threadChicNetClient.Init(ADDR_SERVER, device, "Receiver", "", onReceive)){

		printf("server connection is successful \n");
		threadChicNetClient.AddReceiveChannel(CHANNEL_VIDEO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_AUDIO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_HAPTIC);

		threadChicNetClient.startThread();
	}
	/*
	ofxThreadChicNetClient2 netClientForRTCP;
	std::map<int, std::string> device1;
	device1[0] = "video";
	if(netClientForRTCP.Init("114.70.63.186", device1, "RTCPReceiver", "", onReceiveRTCP)){

		printf("server connection is successful \n");
		threadChicNetClient.AddReceiveChannel(CHANNEL_VIDEO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_AUDIO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_HAPTIC);
		netClientForRTCP.startThread();
	}
	*/

	//CMediaCodec mediaEncoder;
	
	//char deviceName[] = "video=Logitech Webcam 905:audio=Microphone(Webcam 905)";

	// initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	//if(!mediaEncoder.initialize(MediaStreamServer, deviceName, AV_CODEC_ID_MPEG1VIDEO, IMG_WIDTH, IMG_HEIGHT, 1000000, 10)){
	//	printf("codec initialization failure \n");
	//	return -1;
	//}


	if(!mediaDecoder.initialize(MediaStreamClient, NULL, AV_CODEC_ID_MPEG1VIDEO)){
		printf("codec initialization failure \n");
		return -1;
	}

	// Decoder
	AVCodec  *audioCodec = avcodec_find_decoder(AV_CODEC_ID_FIRST_AUDIO);
	AVCodecContext* aCodecCtx = avcodec_alloc_context3(audioCodec);
	aCodecCtx->sample_rate = 44100;
	aCodecCtx->bit_rate = 1411200;
	aCodecCtx->channels = 1;
	aCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
	if(avcodec_open2(aCodecCtx, audioCodec, NULL) < 0){
		fprintf(stderr, "fail to open decoder \n");
	}

	SDL_AudioSpec wanted_spec, spec;
	wanted_spec.freq = 44100;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 1;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = aCodecCtx;

	if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
	}

	
	SDL_PauseAudio(0);
	
	//SDL_CreateThread(decode_thread, (void*)&mediaDecoder);

	hav::packet_queue_init(&gVideoQueue);
	hav::packet_queue_start(&gVideoQueue);
	hav::packet_queue_init(&gAudioQueue);
	hav::packet_queue_start(&gAudioQueue);

	SDL_CreateThread(decode_thread, (void*)&mediaDecoder);

	// Allocate a place to put our YUV image on that screen
	gScreen = SDL_SetVideoMode(IMG_WIDTH, IMG_HEIGHT, 0, 0);

	for(;;){

		
		SDL_Event event;	
		SDL_PollEvent(&event);
		switch(event.type){
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		default:
			break;
		}

	}

	threadChicNetClient.stopThread();
	
	return 0;
}

