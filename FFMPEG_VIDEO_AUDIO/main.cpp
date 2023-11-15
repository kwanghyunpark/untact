#include "ofMain.h"
#include "testApp.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main(int argc, char* argv[] ){


	//int64_t mNtpTimeOffset = 0;
	//CNTPSynchronizer ntpSynch(SERVER_IP_ADDRESS, 500);
	//ntpSynch.runNTPSynchronization();
	//int64_t mNtpTimeOffset = ntpSynch.getOffset();

	ofAppGlutWindow window;
	ofSetupOpenGL(&window, VIDEO_WIDTH,VIDEO_HEIGHT, OF_WINDOW);			// <-------- setup the GL context
	ofSetFrameRate(30);
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	testApp* app = new testApp();
	
	//app->setoffset(mNtpTimeOffset);
	//app->setoffset(mNtpTimeOffset);


	ofRunApp(app);
	return 0;
}
