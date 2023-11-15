#pragma once

#include "ofMain.h"


// #include "ofxUI.h"
// #include "ofVbo.h"
#include "DRAWGL.h"
#include "FFMPEG.h"
//#include "HAPTIC.h"

// By HHKim
#include "../../CommonCodes/VectorAR.h"
#include "../../CommonCodes/HapticDevices.h"
#include <mmsystem.h>



#define SERVER_IP_ADDRESS  "114.70.63.186"
//#define SERVER_IP_ADDRESS  "192.168.2.19"

#define WIDTH  640
#define HEIGHT 480
#define CHANNEL 3

#define VIDEO_WIDTH 1280
#ifdef INPUT_CAPTURE_SIN
#define VIDEO_HEIGHT 480
#endif
#ifdef INPUT_MEDIAFILE 
#define VIDEO_HEIGHT 720
#endif



class testApp : public ofBaseApp{


private:

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

	//-----------------------------------------------

	ofTexture	   mGlImageTexture;			// texture for display

	//HAPTIC haptic;
	FFMPEG ffmpeg;
	DRAWGL drawgl;

	void copyFrameDataToRgbBuffer(AVFrame* decodedFrame);
	unsigned char mRgbBuffer[VIDEO_WIDTH*VIDEO_HEIGHT*3];
	ofxUICanvas *gui1;
	void setGUI1();	void guiEvent(ofxUIEventArgs &e);

	
	
	bool saveFlag;
	int saveCnt;

	void audioOut(float * input, int bufferSize, int nChannels);
	ofSoundStream soundStream;
	int		sampleRate;
	float 	volume;

	vector <float> lAudio;
	vector <float> rAudio;
	
	int64_t timeoffset;
	void setoffset(int64_t t){
		timeoffset = t;
	}

	

	float 	targetFrequency;
	float 	phase;
	float 	phaseAdder;
	float 	phaseAdderTarget;
	
};
