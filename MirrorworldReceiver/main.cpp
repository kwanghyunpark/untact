
#ifdef NDEBUG
	#ifdef _DEBUG
		#undef _DEBUG
	#endif
#endif

#include "ofMain.h"
#include "testApp.h"
#include "ofAppGlutWindow.h"

//========================================================================
//int main( ){


int main(int argc, char* argv[])
{
    ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1900,1000, OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp( new testApp());

	return -1;
}
