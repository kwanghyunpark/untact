#pragma once
#include "ofxUI.h"
#include "ofVbo.h"
#include "glut.h"

#define NUM_BILLBOARDS 100
class DRAWGL
{
public:
	// By HHKim
	void DRAWGL::drawHapticScene(double* pos1, double* ballState);

	DRAWGL(void);
	~DRAWGL(void);
// 
	void bset();
	void bdraw();
	void bupdate();	
		float billboardSizeTarget[NUM_BILLBOARDS];
	ofShader billboardShader;
	ofImage texture;
	ofVboMesh billboards;
	ofVec3f billboardVels[NUM_BILLBOARDS];	void drawViewportOutline(const ofRectangle & viewport);
	void setProperLight();
	void Canvas1(double*, double*, double*, double);
	ofRectangle Rect1;
	ofEasyCam camera1;
};

