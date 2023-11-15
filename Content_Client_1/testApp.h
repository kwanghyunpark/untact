#pragma once

#include "ofMain.h"
#include "MediaCodec.h"
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
#include "ofUtils.h"
#include "ofxThreadChicNetClient.h"
#include <hdl/hdl.h>
#include <hdlu/hdlu.h>
#include "../../CommonCodes/hav_fifo.h"
#include "glut.h"
#include "ofxUI.h"
#include "ofVbo.h"
#include "NTPSynchronizer.h"

#include "../../CommonCodes/VectorAR.h"
#include "../../CommonCodes/HapticDevices.h"
#include <windows.h>
#include "stdint.h"


#define SERVER_IP_ADDRESS  "114.70.63.28"
//#define SERVER_IP_ADDRESS  "192.168.1.5"

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

#define WIDTH  640
#define HEIGHT 480
#define CHANNEL 3

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

#define SERIAL_WAIT 20
#define SOUND_WAIT 60
#define SPHERE_SIZE 200
#define SPHERE_JUMP 300
#define FINGER_SPHERE_SIZE 10


const bool bNonBlocking = false;
const bool bBlocking = true;

typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

enum RS_Face {
	FACE_NONE = -1,
	FACE_NEAR, FACE_RIGHT, 
	FACE_FAR, FACE_LEFT, 
	FACE_TOP, FACE_BOTTOM,
	FACE_DEFAULT,
	FACE_LAST               // reserved to allow iteration over faces
};

struct CodecData{
	int audio_sample_rate;
	int audio_bit_rate;
	int video_sample_rate;
	int vodeo_bit_rate;
	int etcData;
};

union CodecPacket{
	CodecData val;
	unsigned char udata[sizeof(CodecData)];
};



class testApp : public ofBaseApp{

	//Haptic Callback 
	friend HDLServoOpExitCode falcon_update(void* pUserData);

	
	private:

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


		ofxThreadChicNetClient mNetSender;
		static bool __stdcall onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);


		ofTexture	   mGlImageTexture;			// texture for display

		unsigned char mRgbBuffer[VIDEO_WIDTH*VIDEO_HEIGHT*3];
		
		// By KHH ----------------------
		HapticWorld hapticDevices;
		void testApp::drawHapticScene(double* pos1, double* ballState);
		void testApp::CheckHapticButtonInput();
		//--------------------------------


	public:

		CMediaCodec mediaDecoder;
		hav::PacketQueue mVideoQueue;
		hav::PacketQueue mAudioQueue;
		hav::PacketQueue mSyncAudioQueue;
		hav::HapticQueue mSyncHapticQueue;
		hav::PacketQueue mSyncVideoQueue;
		hav::HapticQueue mHapticQueue;

		void setup();
		void update();
		void draw();
		void exit();
		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


		hav::HapticPacket hpkt;
		void updateHapticInfo(hav::HapticPacket packet);
		//-------------------------------------------
		void copyFrameDataToRgbBuffer(AVFrame* decodedFrame);
		ofRectangle Rect1;
		ofEasyCam camera1;
		void drawViewportOutline(const ofRectangle & viewport);
		void drawAxis(float length);
		void Canvas1(double*);
		//-------------------------------------------
		

		//Audio
		void Audio_init();
		AVCodec *AudioDecodcodec;
		AVCodecContext *AudioDecodeContext;	
		void audioOut(float * input, int bufferSize, int nChannels);
		ofSoundStream soundStream;
		int		sampleRate;
		float 	volume;

		vector <float> lAudio;
		vector <float> rAudio;


		//Falcon
		void Falcon_Init();
		void testHDLError(const char* str);
		// Get position
		void getPosition(double pos[3]);
		
public:

	// Matrix multiply
	void vecMultMatrix(double srcVec[3], double mat[16], double dstVec[3]);


	// Nothing happens until initialization is done
	bool m_inited;

	// Transformation from Device coordinates to Application coordinates
	double m_transformMat[16];

	// Variables used only by servo thread
	double m_positionServo[3];
	// Variables used only by application thread
	double m_positionApp[3];

	// Handle to device
	HDLDeviceHandle m_deviceHandle;

	// Handle to Contact Callback 
	HDLServoOpExitCode m_servoOp;

	// Device workspace dimensions
	double m_workspaceDims[6];

	// Stiffness of cube
	double m_cubeStiffness;
	double gforce[3];
	bool gtouch;
	double gBallPosition[3];
	double gBallVelocity[3];
	double gBallMass;
	double gBallViscosity;
	double gBallStiffness;
	double gCursorRadius;
	double gBallRadius;
	double gLastTime ;
	double gCursorScale;
	GLuint gCursorDisplayList;
	
	double getSystemTime();

	//GUI
	ofxUICanvas *gui1;
	void setGUI1();	void guiEvent(ofxUIEventArgs &e);

	int64_t sync_error_time;
	int priorpacketCnt;
	//---------------------------
	FILE *errfile;


	FILE *VideoTimeFile;
	FILE *AudioTimeFile;
	FILE *HAPTICTimeFile;


	int64_t timeoffset;
	void setoffset(int64_t t){
		timeoffset = t;
	}

	bool Save_flag;
	int buff_flag;
	ofxUIMovingGraph *mg;
	ofxUILabel *tex;

	int64_t dt;
	int64_t t_i;
	int64_t t_j;
	int64_t T_i;
	int64_t T_j;

};
