#pragma once

#include "ofMain.h"
#include "DRAWGL.h"
#include "../../CommonCodes/VectorAR.h"
#include "../../CommonCodes/HapticDevices.h"

using namespace std;

#define LS_NONE 0
#define LS_ORIG 4
#define LS_CAPTURE 1
#define LS_TRAINING 2
#define LS_PREDICTION 3


class testApp : public ofBaseApp{
private:
	
	DRAWGL drawgl;

	//HDContainer g_device;
	HapticWorld hapticDevices;

	public:
		void testApp::CheckHapticButtonInput();
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
		
};
