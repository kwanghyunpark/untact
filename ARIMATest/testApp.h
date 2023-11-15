#pragma once

#include "ofMain.h"
#include "ofxSimpleSlider.h"

#define NOISE_DATA_STRIP_LENGTH 256

struct NoiseDataStrip {
	float data[NOISE_DATA_STRIP_LENGTH];
	float x;
	float y;
	float width; 
	float height; 
	float noiseStep; 
};

struct PlotWnd{
	float data[NOISE_DATA_STRIP_LENGTH];
	float predictedData[NOISE_DATA_STRIP_LENGTH/4];
	float predictionRMS[NOISE_DATA_STRIP_LENGTH/4];
	float originData[NOISE_DATA_STRIP_LENGTH + NOISE_DATA_STRIP_LENGTH/4];
	float x;
	float y;
	float width; 
	float height; 
	float noiseStep; 
	float ymax;
	float ymin;
};


class testApp : public ofBaseApp{
private:
	PlotWnd mPlot;
	ofxSimpleSlider slider;

public:
	void	setup();
	void	update();
	void	draw();

	void	setupMultibandNoiseDemo();
	void	renderMultibandNoiseDemo();
	void 	updateMultibandNoiseDemo();


	//---------------------------------------
	void	keyPressed(int key);
	void	keyReleased(int key);
	void	mouseMoved(int x, int y );
	void	mouseDragged(int x, int y, int button);
	void	mousePressed(int x, int y, int button);
	void	mouseReleased(int x, int y, int button);
	void	windowResized(int w, int h);
	void	dragEvent(ofDragInfo dragInfo);
	void	gotMessage(ofMessage msg);

	float	originX; 
	float	originY;

	void	render1DNoiseStrip (float x, float y, float width, float height, float dt, float *data);
	void	render1DNoiseStrip (PlotWnd graph);

	float	summedNoiseData[NOISE_DATA_STRIP_LENGTH];
	int		nNoiseStrips;	
	vector<ofxSimpleSlider> sliderGroup;
	vector<NoiseDataStrip> noiseDataStripGroup;
		
};
