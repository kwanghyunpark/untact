#pragma once

#include "ofMain.h"

#include "ofxThreadChicNetClient.h"

#include "NTPSynchronizer.h"
// #include "ofxUI.h"
// #include "ofVbo.h"
#include "DRAWGL.h"
#include "FFMPEG.h"
//#include "HAPTIC.h"

// By HHKim
#include "../../CommonCodes/VectorAR.h"
#include "../../CommonCodes/HapticDevices.h"
#include <mmsystem.h>


#define SERVER_TO_CLIENT_1_VIDEO_CHANNEL 1001
#define SERVER_TO_CLIENT_1_AUDIO_CHANNEL 1002
#define SERVER_TO_CLIENT_1_HAPTIC_CHANNEL 1003

#define SERVER_TO_CLIENT_2_VIDEO_CHANNEL 1004
#define SERVER_TO_CLIENT_2_AUDIO_CHANNEL 1005
#define SERVER_TO_CLIENT_2_HAPTIC_CHANNEL 1006

#define CLIENT_1_TO_SERVER_VIDEO_CHANNEL 1007
#define CLIENT_1_TO_SERVER_AUDIO_CHANNEL 1008
#define CLIENT_1_TO_SERVER_HAPTIC_CHANNEL 1009

#define CLIENT_2_TO_SERVER_VIDEO_CHANNEL 1010
#define CLIENT_2_TO_SERVER_AUDIO_CHANNEL 1011
#define CLIENT_2_TO_SERVER_HAPTIC_CHANNEL 1012

#define CLIENT_1_TO_SERVER_CODEC_CHANNEL 1013
#define CLIENT_2_TO_SERVER_CODEC_CHANNEL 1014

#define SERVER_IP_ADDRESS  "114.70.63.28"
//#define SERVER_IP_ADDRESS  "192.168.1.2"

#define WIDTH  640
#define HEIGHT 480
#define CHANNEL 3

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

struct CodecData{
	int audio_sample_rate;
	int audio_bit_rate;
	int video_sample_rate;
	int video_bit_rate;
	int etcData;
};

union CodecPacket{
	CodecData val;
	unsigned char udata[sizeof(CodecData)];
};

// struct HapticData{
// 	double pos[3];
// 	double ballpos[3];
// 	double Force[3];
// 	bool touch;
// };
// union HapticPacket{
// 	HapticData val;
// 	unsigned char udata[sizeof(HapticData)];
// };

class testApp : public ofBaseApp{

//  ==========  HHKim  ============ 
private:
	HapticWorld hapticDevices;
	void testApp::CheckHapticButtonInput();
// -------------------------------
	// Task scheduler -------
	HANDLE handleHapticDataSerder;
	void setTimerEventScheduler();
	void killTimerEventScheduler();
	static void CALLBACK _intraQueueTimerEventListenerForHapticDataSender(void* lpParameter, BOOLEAN TimerOrWaitFired){
		testApp* obj = (testApp*) lpParameter;
		obj->queueTimerEventListenerForHapticDataSender();
	}
	void queueTimerEventListenerForHapticDataSender();
	// End of Task scheduler -------

public:
	void setup();
	void update();
	void draw();

	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	//---------ChicServer----------------------------
	ofxThreadChicNetClient mNetSender;
	static bool __stdcall onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);
	ofxThreadChicNetClient* getSocket(){return &mNetSender;};
	//-----------------------------------------------

	ofTexture	   mGlImageTexture;			// texture for display
	
	//HAPTIC haptic;
	FFMPEG ffmpeg;
	DRAWGL drawgl;
	

	hav::HapticPacket Client1_packet;
	hav::HapticPacket Client2_packet;

	CodecPacket Client1_Codec_packet;
	CodecPacket Client2_Codec_packet;

	void updateHapticInfo(hav::HapticPacket packet, int channel);
	void updateCodecInfo(CodecPacket packet, int channel);
	
	ofxUICanvas *gui1;
	void setGUI1();	void guiEvent(ofxUIEventArgs &e);

	double penetrationDist1;
	double penetrationDist2;

	bool haptic_f1;
	bool haptic_f2;

	FILE* hapticSaveFile;
	bool saveFlag;
	int saveCnt;
	
	void audioOut(float * input, int bufferSize, int nChannels);
	ofSoundStream soundStream;
	int		sampleRate;
	float 	volume;

	vector <float> lAudio;
	vector <float> rAudio;
	AVCodec *AudioDecodcodec;
	AVCodecContext *AudioDecodeContext;	
	void Audio_init();
	SwrContext* swr_ctx; 


	int64_t timeoffset;
	void setoffset(int64_t t){
		timeoffset = t;
	}

	hav::HapticQueue mReceiveQueue;


};
