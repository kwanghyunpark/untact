#ifndef _TEST_APP
#define _TEST_APP


#include "ofMain.h"
#include "ofxThreadChicNetClient.h"
#include "../../apps/ChicMediaNetwork/FFMpegMediaDll/MediaCodec.h"
#include "../../CommonCodes/NTPSynchronizer.h"
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
#include "ofUtils.h"
#include "../../CommonCodes/MirrorworldConfig.h"
#include "ofxSimpleSerial.h"
#include "Leap.h"
#include "SampleListener.h"


class testApp : public ofBaseApp{

private:

	ofxThreadChicNetClient mNetSender;
	static bool __stdcall onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);
	std::queue<AudioPacket> mAudioPacketQueue;
	std::queue<HapticPacket>mHapticPacketQueue;
	
public:
	CMediaCodec mediaDecoder;
	hav::PacketQueue mVideoQueue;
	//hav::PacketQueue mAudioQueue;

	std::queue<AudioPacket> *getAudioPacketQueue(){return &mAudioPacketQueue;};
	std::queue<HapticPacket>*getHapticPacketQueue(){return &mHapticPacketQueue;};
	
	
	ofTexture	   mGlImageTexture;			// texture for display

	unsigned char mRgbBuffer[VIDEO_WIDTH*VIDEO_HEIGHT*3];
	void copyFrameDataToRgbBuffer(AVFrame* decodedFrame);

	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	//void audioIn(float * input, int bufferSize, int nChannels); 
	void audioOut(float * input, int bufferSize, int nChannels);
	void drawViewportOutline(const ofRectangle & viewport);
	void updateHapticInfo(HapticPacket packet);

	//-------------LeapDataDraw----------
	void canvas(HapticPacket packet);
	void Canvas3(Ldata &ldata);
	void Canvas1(Ldata &ldata);
	void DrawParticle(HapticPacket packet);
	void SerialSend(int, float);
	void onNewMessage(string & message);
	void BounceCircle(HapticPacket packet);

	ofRectangle viewport3D3;
	ofRectangle viewport3D4;
	ofxSimpleSerial	serial;
	ofEasyCam camera2;
	ofEasyCam camera3;


	SampleListener listener;
	Controller controller;
	Ldata* ldata;
	
	int hexcolor[5];
	int serial_delay_cnt1;
	int serial_delay_cnt2;
	int npos1;
	int npos2;
	int oldpos1;
	int oldpos2;
	ofSoundPlayer synth;
	int sound_cnt;
	int Inside_cnt1;
	int Inside_cnt2;
	float counter;
	float InsideVec1[5000][3];
	
	HapticPacket hpkt;
	BOOL serial_f;
	//-----------------------------------
	vector <float> left;
	vector <float> right;
	vector <float> volHistory;

	int 	bufferCounter;
	int 	drawCounter;

	float smoothedVol;
	float scaledVol;

	ofSoundStream soundStream;
};

#endif	

