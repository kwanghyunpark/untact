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
};

#endif

#include <stdio.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
#include "../../apps/ChicMediaNetwork/FFMpegMediaDll/MediaCodec.h"
#include "ofxThreadChicNetClient.h"
#include "ofUtils.h"
#include "TaskForSyncNetworkTime.h"

hav::PacketQueue* gVideoQueue;
hav::PacketQueue* gAudioQueue;
SDL_Surface* gScreen;

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define SDL_AUDIO_BUFFER_SIZE 1024
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

/*
int encode_thread(void* data) 
{
	encodeThreadData* _data = (encodeThreadData*) data;
	CMediaCodec* mediaCodec = _data->codec;
	ofxThreadChicNetClient2* threadChicNetClient = _data->threadClient;
	//CMediaCodec* mediaCodec = (CMediaCodec*) data;

	AVPacket encoded_packet;
	for(;;) {
		Sleep(5);

		AVFrame* aFrame = mediaCodec->captureVideoFrame();
		if(aFrame != NULL){

			AVPacket encoded_packet;
			if( mediaCodec->encodeVideoFrameToPacket(aFrame, encoded_packet) > 0){
				packet_queue_put(&gVideoQueue, &encoded_packet);
				mediaCodec->renderVideoFrame(aFrame, gScreen, mediaCodec->getVideoCapturerContext());
				
				threadChicNetClient->SendData("Receiver",(char*)encoded_packet.data, encoded_packet.size,CHANNEL_VIDEO);

				//threadChicNetClient->SendData("Receiver",(char*)encoded_packet.data, encoded_packet.size,1000);

			}else{
				av_free_packet(&encoded_packet);	
			}
		}
		av_free(aFrame);
	}
	return 0;
}
*/

bool __stdcall onReceive(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);

double getCurrentSystemTimeMicros()
{
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time

	// get the high resolution counter's accuracy
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);
	return (1000000.0*(double)tick.QuadPart / (double)ticksPerSecond.QuadPart);
	//printf ("\n Ticks par second = %.0f",microSecondes);
}


int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;
	static AVFrame frame;

	int len1, data_size = 0;
	int64_t time_stamp;
	for(;;) {

		while(audio_pkt_size > 0) {

			int got_frame = 0;
			len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
			if(len1 < 0) {
				/* if error, skip frame */
				audio_pkt_size = 0;
				break;
			}
			audio_pkt_data += len1;
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

		int64_t ctime = av_gettime();
		double dt = 0.02;

		//av_init_packet(&pkt);
		if((double)(ctime - gAudioQueue->time_stamp)*0.000001 > dt+0.0001){
			if(hav::packet_queue_get(gAudioQueue, &pkt, 1, &time_stamp) < 0) {
				return -1;
			}
			if((double)(ctime - time_stamp)*0.000001 < dt-0.0001){
				audio_pkt_data = pkt.data;
				audio_pkt_size = pkt.size;
			}
			printf("%lld - %lld = %lld \n",ctime , time_stamp, ctime-time_stamp);
			//printf("%lld\n",gAudioQueue.time_stamp);
		}
	}
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
			audio_size = audio_decode_frame(aCodecCtx, audio_buf, audio_buf_size);
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

static int decode_thread(void* data) 
{
	CMediaCodec* decoder = (CMediaCodec*)data;


	for(;;){

		AVPacket packet;
		if(gVideoQueue->nb_packets > 1){
			int64_t time_stamp;

			int64_t ctime = av_gettime();
			double dt = 0.1;

			if((double)(ctime - gVideoQueue->time_stamp)*0.000001 > dt+0.0001){
				if(hav::packet_queue_get(gVideoQueue, &packet, 1, &time_stamp) < 0) {
					return -1;
				}

				if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
					AVFrame* decodedFrame = decoder->decodeVideoPacketToFrame(packet);
//					printf("%lld - %lld = %lld \n",ctime , time_stamp, ctime-time_stamp);
					if(decodedFrame != NULL){
						if((double)(ctime - time_stamp)*0.000001 < dt-0.0001){
							decoder->renderVideoFrame(decodedFrame, gScreen, decoder->getVideoDecoderContext(), IMG_WIDTH);
						}
						av_free(decodedFrame);	
					}
				}
				av_free_packet(&packet);
			}
		}
	}
	return 0;
}


static int dataSendingThread(void* data)
{
	ofxThreadChicNetClient* client = (ofxThreadChicNetClient*) data;
	for(;;){
		
		if(gAudioQueue->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(gAudioQueue, &packet, 1, &time_stamp) < 0) {
				return -1;
			}
			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				client->SendData("Receiver", (char*)buffer, size, CHANNEL_AUDIO);
				//printf("audio %d, ", packet.size);	
				av_free(buffer);
			}
		}
		//Sleep(1);

		if(gVideoQueue->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(gVideoQueue, &packet, 1, &time_stamp) < 0) {
				return -1;
			}

			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				client->SendData("Receiver", (char*)buffer, size, CHANNEL_VIDEO);
				//printf("video %d \n", packet.size);
				//printf("%d => %lld \n", gVideoQueue->nb_packets, time_stamp);
				av_free(buffer);
			}
		}
		//Sleep(1);
	}
}


static int dataSendingThread_RTCP(void* data)
{
	ofxThreadChicNetClient* client = (ofxThreadChicNetClient*) data;
	for(;;){

		if(gAudioQueue->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(gAudioQueue, &packet, 1, &time_stamp) < 0) {
				return -1;
			}
			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				client->SendData("RTCPReceiver", (char*)buffer, size, CHANNEL_AUDIO);
				//printf("audio %d, ", packet.size);	
				av_free(buffer);
			}
		}
		/*
		if(gVideoQueue->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(gVideoQueue, &packet, 1, &time_stamp) < 0) {
				return -1;
			}

			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				client->SendData("Receiver", (char*)buffer, size, CHANNEL_VIDEO);
				//printf("video %d \n", packet.size);
				//printf("%d => %lld \n", gVideoQueue->nb_packets, time_stamp);
				av_free(buffer);
			}
		}
		*/
	}
}

bool __stdcall onReceiveRTCP(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	static int cnt = 0;

	if(nBufLen<=0)return false;
	char* packetData = (char*)pBuf;

	//if(nChannel != 1000) return false;

	if(nBufLen != 0){
		printf("received ==> ");
		for(int i=0; i< nBufLen; i++){
			printf("%c", packetData[i]);
		}
		printf("\n");
	}
	return true;
}

int main(int argc, char* argv[])
{
	CNTPSynchronizer ntpTimer;
	ntpTimer.runNTPSynchronization();

	//for(;;){
	//	printf("local : %lld, global: %lld \n", av_gettime(), ntpTimer.getNTPTime());
	//	Sleep(100);
	//}

	ofxThreadChicNetClient threadChicNetClient;
	std::map<int, std::string> device;
	device[0] = "video";
	if(threadChicNetClient.Init(ADDR_SERVER, device, "Sender", "", onReceive)){

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
	if(netClientForRTCP.Init("114.70.63.186", device1, "RTCPSender", "", onReceiveRTCP)){

		printf("server connection is successful \n");
		threadChicNetClient.AddReceiveChannel(CHANNEL_VIDEO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_AUDIO);
		threadChicNetClient.AddReceiveChannel(CHANNEL_HAPTIC);
		netClientForRTCP.startThread();
	}
	*/
	//CMediaCodec mediaCodec;
	CMediaCodec mediaEncoder;
	CMediaCodec mediaDecoder;

	char deviceName[] = "video=Logitech Webcam 905:audio=Microphone(Webcam 905)";
	
	// initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	if(!mediaEncoder.initialize(MediaStreamServer, deviceName, AV_CODEC_ID_MPEG1VIDEO, IMG_WIDTH, IMG_HEIGHT, 1000000, 10, ntpTimer.getOffset())){
		printf("codec initialization failure \n");
		return -1;
	}

	if(!mediaDecoder.initialize(MediaStreamClient, NULL, AV_CODEC_ID_MPEG1VIDEO)){
		printf("codec initialization failure \n");
		return -1;
	}
	
	AVCodecContext  *aCodecCtx = mediaEncoder.getAudioCapturerContext();
	/*
	SDL_AudioSpec wanted_spec, spec;
	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = aCodecCtx;

	if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
	}

	
	SDL_PauseAudio(0);
	*/
	gVideoQueue = mediaEncoder.getVideoBuffer();
	gAudioQueue = mediaEncoder.getAudioBuffer();
	mediaEncoder.startMediaCapture();
	//SDL_CreateThread(decode_thread, (void*)&mediaDecoder);

	SDL_CreateThread(dataSendingThread, (void*)&threadChicNetClient);
	//SDL_CreateThread(dataSendingThread_RTCP, (void*)&netClientForRTCP);

	//gScreen = SDL_SetVideoMode(IMG_WIDTH*2, IMG_HEIGHT, 0, 0);
	
	//encodeThreadData _data;
	//_data.codec = &mediaEncoder;
	//_data.threadClient = &threadChicNetClient;
	//SDL_CreateThread(encode_thread, (void*)&_data);

	//SDL_CreateThread(encode_thread, (void*)&mediaEncoder);
	//SDL_CreateThread(decode_thread, (void*)&mediaDecoder);

	double timeStamp = 0;
	for(;;){

		Sleep(1000);
		char txt[100];
		
		double time  = getCurrentSystemTimeMicros();
		//printf("%.0f\n", time);
		sprintf(txt, "%.0f\n", time);
		string data = txt;
		timeStamp += 0.01;
		
		threadChicNetClient.SendData("Receiver", (char*)data.c_str(), data.length(), CHANNEL_HAPTIC);
		//netClientForRTCP.SendData("Receiver", (char*)data.c_str(), data.length(), CHANNEL_RTCP);

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
//	threadChicNetClient.stopThread();
// 
// 		while(1){
// 			threadChicNetClient.SendData("Receiver","init_ok",8,1000);
// 			printf("send to Receiver \n");
// 			Sleep(1000);
// 		}

		

	return 0;
}

bool __stdcall onReceive(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	static int cnt = 0;

	if(nBufLen<=0)return false;
	char* packetData = (char*)pBuf;

	//if(nChannel != 1000) return false;

	if(nBufLen != 0){
		printf("received ==> ");
		for(int i=0; i< nBufLen; i++){
			printf("%c", packetData[i]);
		}
		printf("\n");
	}
return true;
}