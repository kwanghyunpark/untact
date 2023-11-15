#include "ofMain.h"
#include "testApp.h"
#include "ofAppGlutWindow.h"

// error LNK2038: mismatch detected for '\ITERATOR_DEBUG_LEVEL': value '2' doesn't match value '0 in main.obj'_ problem. 
// You have to modify the openFrameworksRelease.props file. 
// Open the file in notepad or something and search for rtAudioD.lib. Change this value into rtAudio.lib 

//========================================================================
//int main( ){

int main(int argc, char* argv[])
{
	int64_t timeOffset = 0;
	CNTPSynchronizer ntpSynch(SERVER_IP_ADDRESS, 500);
	ntpSynch.runNTPSynchronization();
	int64_t mNtpTimeOffset = ntpSynch.getOffset();

	
	ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1900,1000, OF_WINDOW);			// <-------- setup the GL context
	
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	testApp* app = new testApp();
	//app->setMediaQueue(gVideoQueue, gAudioQueue);
	app->setOffset(mNtpTimeOffset);
	ofRunApp(app);
	//new testApp()
	//ofRunApp();

	return -1;
}
