/*
 This example demonstrates how more "organic" noise signals
 can be generated by summing multiple "octaves" of noise.
*/

#include "testApp.h"
#include <stdio.h>
#include "../../CommonCodes/arima_tool.h"
//--------------------------------------------------------------
vector<float> pos_x;
vector<float> pos_y;
vector<float> pos_z;
vector<float> dist;
float idx_start;
float idx_end;


double ar_param[] =
{
	1.258051254149946,
	0.227577311182717,
	-0.258738773793149,
	-0.147970081076526,
	0.001664111920015,
	-0.115861421910991,
	-0.001683762286085,
	0.001195693456229,
	-0.100830448025619,
	0.132442431599762
};

double y[]={
	88.995293000000004,
	92.642166000000003,
	94.392807000000005,
	94.869179000000003,
	94.783867000000001,
	93.506400999999997,
	90.911582999999993,
	87.830878999999996,
	84.190651000000003,
	82.197425999999993,
	78.231316000000007,
	74.348450000000000,
	72.510727000000003,
	69.057243000000000,
	67.531738000000004,
	63.265563999999998,
	61.500435000000003,
	57.994053000000001,
	54.590656000000003,
	51.264190999999997
};


ARIMAModel arima;

void testApp::setup(){
	
	FILE *fp = fopen("../TestData.txt", "r");
	if(fp != NULL){
		while(!feof(fp)){
			float a, b, c, d;
			fscanf(fp, "%f, %f, %f, %f", &a, &b, &c, &d);
			pos_x.push_back(a);
			pos_y.push_back(b);
			pos_z.push_back(c);
			dist.push_back(d);
		}
		fclose(fp);
	}
	
	/*
	for(int i=0; i<10; i++){
	arima.ar_params.push_back(ar_param[i]);
	}
	arima.intercept = 0.067000893254963;
	arima.sigma_2 = 0.129540203581745;
	arima.d = 0;
	*/

	// for ARIMA(5,0,2)
	double ar_param2[] = {
		1.999999800000000,
		-0.554131368843494,
		-0.582488399527022,
		-0.195277126127169,
		0.331437105446567
	};
	double ma_param2[] = {
		-0.714751036259691,
		-0.136846430923180
	};
	for(int i=0; i<5; i++){
		arima.ar_params.push_back(ar_param2[i]);
	}
	for(int i=0; i<2; i++){
		arima.ma_params.push_back(ma_param2[i]);
	}
	arima.intercept = 0.007182133348875;
	arima.sigma_2 = 0.133457137573684;
	arima.d = 0;

	ofSetWindowTitle("Multiband Noise Example");
	ofSetVerticalSync(true);
	
	//setupMultibandNoiseDemo();

	mPlot.width =  800;
	mPlot.height = 600;
	mPlot.x = 20;
	mPlot.y = 20;
	mPlot.noiseStep = 0.02;
	mPlot.ymin = -100;
	mPlot.ymax =  100;
	for(int i=0; i<NOISE_DATA_STRIP_LENGTH; i++){
		if(i< NOISE_DATA_STRIP_LENGTH/4){
			mPlot.predictedData[i]= 0;
		}
		mPlot.data[i] = 0;
	}
	
	slider.setup(mPlot.x + mPlot.width + 10, mPlot.y, 16, mPlot.height, 0, 100, 50, true,true);
}

//--------------------------------------------------------------
void testApp::update(){
	//updateMultibandNoiseDemo();

	// Push the older data to the end of the array
	float *data = (float *)mPlot.data;
	for (int j=(NOISE_DATA_STRIP_LENGTH-1); j>0; j--){
		data[j] = data[j-1];
	}
	float noiseStep = mPlot.noiseStep; 
	float t = (ofGetElapsedTimeMillis()/10.0) * noiseStep;
	
	
	static int cnt=0;
	if(cnt >= pos_x.size()){
		cnt = 0;
	}
	data[0] = pos_x[cnt];

	
	for(int i= 0; i<NOISE_DATA_STRIP_LENGTH; i++){
		mPlot.originData[i] = pos_x[i+cnt];
	}

	int nObserv = 3*NOISE_DATA_STRIP_LENGTH/4;

	if(cnt > nObserv){
		unsigned int N = NOISE_DATA_STRIP_LENGTH/4;
		std::vector<double> observations;

		int start_idx = cnt-N-nObserv;
		for(int i= max(0,start_idx); i< cnt-N; i++){
			observations.push_back(pos_x[i]);
		}
		Predictions output = ARIMAForecaster::forecast(arima, N, observations);
		for(int i=0; i<N; i++){
			mPlot.predictedData[i] = output.values[i];
			mPlot.predictionRMS[i] = output.error[i];
		}
		//for(int i=0; i< output.values.size(); i++){
		//	printf("%f \n", output.values[i]);
		//}
	}
	
	//for(int i=cnt;)


	cnt++;
}

//--------------------------------------------------------------
void testApp::draw(){
	ofBackgroundGradient( ofColor(240), ofColor(180), OF_GRADIENT_CIRCULAR);
	//renderMultibandNoiseDemo();

	render1DNoiseStrip(mPlot);

}

//--------------------------------------------------------------
void testApp::setupMultibandNoiseDemo(){
	// setup and allocate resources used in the multi-noise strip demo.
	
	nNoiseStrips = 8;
	sliderGroup.resize(nNoiseStrips); 
	noiseDataStripGroup.resize(nNoiseStrips); 
	
	originX = 100; 
	originY = 100; 
	float stripWidth	= 300;
	float stripHeight	= 35;
	float yMargin		= 5;
	float stripXPos		= originX;
	float stripYPos		= originY;
	float noiseStep		= 0.001;
	
	// These are the initial weights of the sliders, which 
	// multiply against their respective noise channels. 
	float noiseAmounts[8] = {0,0,0, 0.82,0.59,0.41, 0.06,0.17}; // fer example
	
	// I'm using a simple struct ("NoiseDataStrip", in testApp.h) to store the 
	// data contained by one of these noise recordings. Each struct contains
	// the bounding coordinates (x,y,w,h), and some other parameters, plus
	// the float array (data) containing the noise recordings. 
	//
	for (int i=0; i<nNoiseStrips; i++){
		noiseDataStripGroup[i].x = stripXPos;
		noiseDataStripGroup[i].y = stripYPos;
		noiseDataStripGroup[i].width = stripWidth;
		noiseDataStripGroup[i].height = stripHeight;
		noiseDataStripGroup[i].noiseStep = noiseStep; 
		noiseStep *= 2.0; 
		for (int j=0; j< NOISE_DATA_STRIP_LENGTH; j++){
			noiseDataStripGroup[i].data[j] = 1.0; 
		}
		
		float sliderX = stripXPos+stripWidth+yMargin;
		float sliderY = stripYPos;
		float sliderAmount = noiseAmounts[i]; //1.0 / (powf(2.0, i));
		sliderGroup[i].setup(sliderX, stripYPos, 16,stripHeight, 0.00, 1.0, sliderAmount, true,true);
		stripYPos += stripHeight + yMargin;
	}

}

//--------------------------------------------------------------
void testApp::updateMultibandNoiseDemo(){
	
	// For each noise strip
	for (int i=0; i<nNoiseStrips; i++){
		
		// Push the older data to the end of the array
		float *data = (float *)noiseDataStripGroup[i].data;
		for (int j=(NOISE_DATA_STRIP_LENGTH-1); j>0; j--){
			data[j] = data[j-1];
		}
		
		// Add the most recent data, the noise value. 
		// Here's where we actually fetch the noise, using ofNoise().
		// Note how ofNoise() requires an argument ('t'); this is
		// the coordinate (on a one-dimensional axis) whose
		// corresponding noise value we wish to obtain. 
		float noiseStep = noiseDataStripGroup[i].noiseStep; 
		float t = (ofGetElapsedTimeMillis()/10.0 + i) * noiseStep;
		data[0] = ofNoise(t);
	}
	
	// Compute the normalization factor: the total sum of the weights
	// for all of the contributing noise channels. This number is the largest
	// value which the sum of noise streams could possibly achieve.
	float normalizationFactor = 0;
	for (int i=0; i<nNoiseStrips; i++){
		float weight = sliderGroup[i].getValue();
		normalizationFactor += weight;
	}
	if (normalizationFactor == 0){
		normalizationFactor = 1.0;
	}
	
	// For every sample in the recording history,
	for (int j=0; j<NOISE_DATA_STRIP_LENGTH; j++){
		float sumj = 0; 
		
		// Sum the weighted contribution from each of the noise strips. 
		for (int i=0; i<nNoiseStrips; i++){
			float val = noiseDataStripGroup[i].data[j];
			float weight = sliderGroup[i].getValue();
			sumj += (weight * val); 
		}
		
		// Normalize it to the range 0...1 by dividing it
		// by normalizationFactor, as we discussed above. 
		summedNoiseData[j] = sumj / normalizationFactor;
	}
	
	
}


//--------------------------------------------------------------
void testApp::renderMultibandNoiseDemo(){
	
	// draw the individual strips
	float stackBottom = 0; 
	for (int i=0; i<nNoiseStrips; i++){
		float x = noiseDataStripGroup[i].x;
		float y = noiseDataStripGroup[i].y;
		float w = noiseDataStripGroup[i].width;
		float h = noiseDataStripGroup[i].height;
		stackBottom = y+h;
		
		float noiseStep = noiseDataStripGroup[i].noiseStep; 
		float *data = (float *) (noiseDataStripGroup[i].data);
		render1DNoiseStrip (x,y, w,h, noiseStep, data); 
	}
	
	// draw the strip containing the summed data. 
	render1DNoiseStrip(originX, stackBottom+125, 300,100, 0, (float *)summedNoiseData);
	
	string multiBandText   = "ofNoise() creates a signal that varies \n";
	multiBandText         += "smoothly between 0 and 1. More 'organic' \n";
	multiBandText         += "noise can be made by adding multiple \n";
	multiBandText         += "'octaves' of noise. The strip below shows \n";
	multiBandText         += "the sum of the above streams, weighted by \n";
	multiBandText         += "the values in their corresponding sliders. \n";	
	
	ofSetColor(0,0,0); 
	ofDrawBitmapString(multiBandText, originX,   stackBottom+33);
	ofDrawBitmapString("ofNoise()",   originX+1, stackBottom+33); //bold it
	
	ofDrawBitmapString("Noise Step", originX-46, originY-5);
	ofDrawBitmapString("Weights", originX+300+5, originY-5);
}


void testApp::render1DNoiseStrip (PlotWnd graph)
{
	float x = graph.x;
	float y = graph.y;
	float width = graph.width;
	float height = graph.height;
	float dt = graph.noiseStep;
	float *data = graph.data;
	float ymin = graph.ymin;
	float ymax = graph.ymax;

	float now = ofGetElapsedTimef();
	ofPushMatrix();
	ofDisableSmoothing();
	ofEnableAlphaBlending();
	ofTranslate(x, y, 0); 
	
	// Yes, this is a drop shadow
	ofFill();
	ofSetColor(0,0,0, 10); 
	ofRect(0,0, width+4, height+2);
	ofRect(0,0, width+2, height+4); 
	
	// Draw a white box underneath the strip
	ofFill();
	ofSetColor(255,255,255); 
	ofRect(0,0, width, height); 
	
	/*
	// Draw a filled gray noise terrain.
	ofEnableSmoothing();
	ofFill();
	ofSetColor(190); 
	ofBeginShape();
	ofVertex(width, height);
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH; i++){
	float px = ofMap(i, 0,(NOISE_DATA_STRIP_LENGTH-1), width,0); 
	float py = height * data[i];
	ofVertex(px,py);
	}
	ofVertex(0, height); 
	ofEndShape(true);
	*/

	ofEnableSmoothing();
	ofFill();
	ofSetColor(190); 
	ofBeginShape();
	ofVertex(width-0.25*width, 0);
	ofVertex(width-0.25*width, height);

	int k = 3*NOISE_DATA_STRIP_LENGTH/4 + 10;
	float px = ofMap(k, 0, (NOISE_DATA_STRIP_LENGTH - 1), 0, width); 
	ofVertex(px, height);
	ofVertex(px, 0);
	ofEndShape(true); 

// 	ofSetColor(0,0,0); 
// 	float error = mPlot.predictedData[5] - mPlot.originData[k];
// 	string label = ofToString(sqrt(error*error));
// 
// 	ofDrawBitmapString(label, width - 50, 50);
// 	printf("%f - %f = %f  \n", mPlot.predictedData[5], mPlot.originData[k], error);
	
	
	//ofSetColor(0,100,0);
	//ofBeginShape();	
		//px = ofMap(k, 0, (NOISE_DATA_STRIP_LENGTH - 1), width, 0); 
		//float py = ofMap(mPlot.originData[k], ymin, ymax, 0, height,0);
		//ofCircle(px,py, 2);
	//ofEndShape(false);
	

	// Draw the black line of the noise waveform
	ofNoFill();
	ofSetColor(0,0,0); 
	ofBeginShape();	
	
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH; i++){
		//float px = ofMap(i, 0,(NOISE_DATA_STRIP_LENGTH-1), width,0); 
		//float px = ofMap(i, 0, (NOISE_DATA_STRIP_LENGTH - 1), width - 0.25*width, 0); 
		float px = ofMap(i, 0, (NOISE_DATA_STRIP_LENGTH - 1), width, 0); 
		float py = ofMap(data[i], ymin, ymax, 0, height,0); 
		if(py > 0 && py < height){
			ofVertex(px,py);
		}
	}
	ofEndShape(false);

	ofSetColor(255,0,0); 
	ofBeginShape();
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH/4; i++){
		float px = ofMap(i,0, (NOISE_DATA_STRIP_LENGTH/4 - 1), width - 0.25*width, width); 
		float py = ofMap(graph.predictedData[i], ymin, ymax, 0, height,0); 
		if(py > 0 && py < height){
			ofVertex(px,py);
		}
	}
	ofEndShape(false); 


	/*
	// draw rms error region
	ofSetColor(255,160,160); 
	ofBeginShape();
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH/4; i++){
		float px = ofMap(i,0, (NOISE_DATA_STRIP_LENGTH/4 - 1), width - 0.25*width, width); 
		float low = graph.predictedData[i] - 1.96*sqrt(graph.predictionRMS[i]);
		float high = graph.predictedData[i] + 1.96*sqrt(graph.predictionRMS[i]);
		float py = ofMap(low, ymin, ymax, 0, height,0); 
		if(py > 0 && py < height){
			ofVertex(px,py);
		}
	}
	ofEndShape(false); 

	ofBeginShape();
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH/4; i++){
		float px = ofMap(i,0, (NOISE_DATA_STRIP_LENGTH/4 - 1), width - 0.25*width, width); 
		float low = graph.predictedData[i] - 1.96*sqrt(graph.predictionRMS[i]);
		float high = graph.predictedData[i] + 1.96*sqrt(graph.predictionRMS[i]);
		float py = ofMap(high, ymin, ymax, 0, height,0); 
		if(py > 0 && py < height){
			ofVertex(px,py);
		}
	}
	ofEndShape(false); 
	*/

	// Draw a box outline on top, around everything
	ofDisableSmoothing();
	ofNoFill();
	ofSetColor(0,0,0); 
	ofRect(0,0, width, height);
	
	// Draw the dt noise-step factor
	if (dt > 0){
		ofSetColor(0,0,0); 
		string label = ofToString(dt);
		ofDrawBitmapString(label, -46, height/2+6);
	}

	ofPopMatrix();
}


//--------------------------------------------------------------
void testApp::render1DNoiseStrip (float x, float y, float width, float height, float dt, float *data){
	
	float now = ofGetElapsedTimef();
	ofPushMatrix();
	ofDisableSmoothing();
	ofEnableAlphaBlending();
	ofTranslate(x, y, 0); 
	
	// Yes, this is a drop shadow
	ofFill();
	ofSetColor(0,0,0, 10); 
	ofRect(0,0, width+4, height+2);
	ofRect(0,0, width+2, height+4); 
	
	// Draw a white box underneath the strip
	ofFill();
	ofSetColor(255,255,255); 
	ofRect(0,0, width, height); 
	
	/*
	// Draw a filled gray noise terrain.
	ofEnableSmoothing();
	ofFill();
	ofSetColor(190); 
	ofBeginShape();
	ofVertex(width, height);
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH; i++){
		float px = ofMap(i, 0,(NOISE_DATA_STRIP_LENGTH-1), width,0); 
		float py = height * data[i];
		ofVertex(px,py);
	}
	ofVertex(0, height); 
	ofEndShape(true);
	*/
	// Draw the black line of the noise waveform
	ofNoFill();
	ofSetColor(0,0,0); 
	ofBeginShape();	
	for (int i=0; i<NOISE_DATA_STRIP_LENGTH; i++){
		float px = ofMap(i, 0,(NOISE_DATA_STRIP_LENGTH-1), width,0); 
		float py = height * data[i];
		ofVertex(px,py);
		
	}
	ofEndShape(false);
	
	// Draw a box outline on top, around everything
	ofDisableSmoothing();
	ofNoFill();
	ofSetColor(0,0,0); 
	ofRect(0,0, width, height);
	
	// Draw the dt noise-step factor
	if (dt > 0){
		ofSetColor(0,0,0); 
		string label = ofToString(dt);
		ofDrawBitmapString(label, -46, height/2+6);
	}


	ofPopMatrix();
}

// In case you're wondering, the simpleSliders get their mouse info through event handlers. 
//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
