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


typedef struct {
	float x;
	float y;
	float z;

}point3D;

class testApp : public ofBaseApp
{
private:
	int64_t mNtpTimeOffset;
	unsigned char mRgbBuffer[VIDEO_WIDTH*VIDEO_HEIGHT*3];
	ofVideoGrabber videoGrabber;
	ofxThreadChicNetClient mNetSender;
	static bool __stdcall onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);
	ofSoundStream soundStream;

	int64_t m_offset;

public:
	void setOffset(int64_t offset){
		m_offset = offset;
	}
	CMediaCodec mediaEncoder;
	CMediaCodec mMediaDecoder;
	ofTexture	   mGlImageTexture;			// texture for display


	//-------Leap--------
	point3D point;
	ofEasyCam camera1;
	ofEasyCam camera3;
	ofRectangle viewport3D1;
	ofRectangle viewport3D3;
	ofRectangle viewport3D4;

	int hexcolor[5];
	float counter;
	HapticPacket hpkt;
	void drawViewportOutline(const ofRectangle & viewport);
	void onNewMessage(string & message);
	void updateHapticInfo(HapticPacket packet);
	void canvas(HapticPacket packet);
	//-------------------
	void copyFrameDataToRgbBuffer(AVFrame* decodedFrame);
	ofxThreadChicNetClient* getSocket(){return &mNetSender;};
	void DrawSphere();
	void DrawFingerPos(HapticPacket);
	void BounceCircle(HapticPacket packet);
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

	void audioIn(float * input, int bufferSize, int nChannels); 
	void audioOut(float * input, int bufferSize, int nChannels);
};

#endif	

