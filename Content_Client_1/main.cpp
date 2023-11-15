#include "ofMain.h"
#include "testApp.h"
#include "ofAppGlutWindow.h"

//========================================================================
int main(int argc, char* argv[] ){
	
	int64_t mNtpTimeOffset = 0;
	
	//CNTPSynchronizer ntpSynch(SERVER_IP_ADDRESS, 500);
	//ntpSynch.runNTPSynchronization();
	//mNtpTimeOffset = ntpSynch.getOffset();
	
    ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1920/2,1080/2, OF_WINDOW);			// <-------- setup the GL context
	window.setFrameRate(40);
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:

	testApp* app = new testApp();
	app->setoffset(mNtpTimeOffset);
	//ofToggleFullscreen(); 
	ofRunApp( app);

	return -1;

}
