#include "testApp.h"

#define _USE_MATH_DEFINES
#include "math.h"
#include "opencv2/opencv.hpp"
#include <mmsystem.h>

using namespace cv;
inline double toRadian(double a)
{ 
	return a*M_PI/180.0;
}

inline double toDegree(double a)
{ 
	return a*180.0/M_PI;
}


// Cube parameters
const double gStiffness = 100.0;
const double gCubeEdgeLength = 0.7;

int64_t av_gettime(void)
{
	FILETIME ft;
	int64_t t;
	GetSystemTimeAsFileTime(&ft);
	t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
	return t / 10 - 11644473600000000; /* Jan 1, 1601 */
}


Mat rotMat(double rx, double ry, double rz)
{
	double cosx = cos(rx);
	double sinx = sin(rx);
	double cosy = cos(ry);
	double siny = sin(ry);
	double cosz = cos(rz);
	double sinz = sin(rz);

	Mat Rx = (Mat_<double>(3,3) <<  1.0f, 0.0f,  0.0f,  
		0.0f, cosx, -sinx,    
		0.0f, sinx,  cosx);
	Mat Ry = (Mat_<double>(3,3) <<  cosy, 0.0f,	siny,
		0.0f, 1.0f,  0.0f, 	 
		-siny, 0.0f,	cosy);
	Mat Rz = (Mat_<double>(3,3) <<  cosz,-sinz, 0.0f, 
		sinz, cosz, 0.0f,
		0.0f, 0.0f, 1.0f);

	return Rz * Ry * Rx;

}

HDLServoOpExitCode g_touchOp = HDL_INVALID_HANDLE;

double force[] = {0,0,0};
double kp = 50;
int64_t oldtime = av_gettime();
double oldposition[] = {0,0,0};
double position[3];

double pos_c1[] = {0,0,0};
double pos_b[] = {0,0,0};
double pos_ball_init[] = {0,0,0};

double p_c2[] = {0,0,0};

Mat x_a(3,1,CV_64FC1, position);
Mat x_b = Mat(Vec3d(0,0,0));
Mat x_o(3,1,CV_64FC1, pos_ball_init);

Mat x_a_old = x_a;
Mat x_b_old = x_b;
Mat x_o_old = x_o;


Mat v_a = Mat(Vec3d(0,0,0));
Mat v_b = Mat(Vec3d(0,0,0));
Mat v_o = Mat(Vec3d(0,0,0));


double X_a[] = {0,0,0};
double V_a[] = {0,0,0};
double X_a_old[] = {0,0,0};

double X_b[] = {0,0,0};
double V_b[] = {0,0,0};
double X_b_old[] = {0,0,0};

//--------------------------------------------------------------
std::list<Vec3d> x_hist;
list<vector<double>> hapticdata_a;
list<vector<double>> hapticdata_b;

double current_time = 0;

VARBatchTrainer batchTRainer(new VARModel(6,3));
bool fCapture;


int fLearningStatus = LS_NONE;

void updatamodel()
{
	
}

/*
HDLServoOpExitCode touchScene(void* pUserData)
{
	
	static unsigned long count = 0;
	
	unsigned long desiredSamplimgTime = 1; 
	if(count % (desiredSamplimgTime) != 0){
		count++;
		return HDL_SERVOOP_CONTINUE;
	}
	count++;

	
	
	int64_t time = av_gettime();
	double dt = (double)(time-oldtime)*0.000001;

	//printf("%f \n", dt);
	
	double XX_a[3];
	double XX_b[3];
	HDLDeviceHandle hd = gHaptics_a.getDeviceHandle();
	hdlMakeCurrent(hd);
	hdlToolPosition(XX_a);


	//hd = gHaptics_b.getDeviceHandle();
	//hdlMakeCurrent(hd);
	//hdlToolPosition(XX_b);

	vector<double> data_a, data_b;
	data_a.push_back(XX_a[0]); data_a.push_back(XX_a[1]); data_a.push_back(XX_a[2]);
	data_b.push_back(XX_b[0]); data_b.push_back(XX_b[1]); data_b.push_back(XX_b[2]);
	
	
	if(fLearningStatus == LS_NONE){
		hapticdata_a.clear();
		hapticdata_b.clear();
	}

	hapticdata_a.push_back(data_a);
	hapticdata_b.push_back(data_b);

	int nnn = 0;
	if(fLearningStatus == LS_PREDICTION || fLearningStatus == LS_ORIG ){
		nnn = 50;
	}

	
	if(hapticdata_a.size() > nnn){
		
		vector<double> rcvdata_a = hapticdata_a.front(); 
		vector<double> rcvdata_b = hapticdata_b.front(); 
		
		double radius = 0.11/2.0;
		double stiffness = 2000;

		double o_mass = 5.0; //10 kg
		double o_viscosity = 150.0;

		X_a[0] = rcvdata_a[0];
		X_a[1] = rcvdata_a[1];
		X_a[2] = rcvdata_a[2];

		X_b[0] = rcvdata_b[0];
		X_b[1] = rcvdata_b[1];
		X_b[2] = rcvdata_b[2];

		for(int i=0; i<3; i++){
			V_a[i] = (X_a[i] - X_a_old[i])/dt;
			V_b[i] = (X_b[i] - X_b_old[i])/dt;
		}

		x_a = rotMat(0.0, toRadian( 90.0), 0.0)*Mat(Vec3d(X_a)) + Mat(Vec3d(0.07,0,0));
		x_b = rotMat(0.0, toRadian(-90.0), 0.0)*Mat(Vec3d(X_b)) - Mat(Vec3d(0.07,0,0));

		double x_o_right = x_o.at<double>(0,0) + radius;
		double diff_a = x_a.at<double>(0,0) - x_o_right;

		double x_o_left = x_o.at<double>(0,0) - radius;
		double diff_b = x_b.at<double>(0,0) - x_o_left;

		double force_b[3];
		double spring = 0.0;
		double damper = 0.0;
		force[0] = -X_a[0] * spring - V_a[0] * damper;
		force[1] = -X_a[1] * spring - V_a[1] * damper;
		force[2] = 0.0;
		if(diff_a < 0){
			force[2] = fabs(diff_a)*stiffness; 
			//if(force[2]>50) force[2] = 30;
		}

		force_b[0] = -X_b[0] * spring - V_b[0] * damper;
		force_b[1] = -X_b[1] * spring - V_b[1] * damper;
		force_b[2] = 0.0;
		if(diff_b > 0){
			force_b[2] = fabs(diff_b)*stiffness;
			//if(force_b[2]>50) force_b[2] = 30;
		}

		Mat F_o = -rotMat(0.0, toRadian( 90.0), 0.0)*Mat(Vec3d(0,0,force[2])) - rotMat(0.0, toRadian(-90.0), 0.0)*Mat(Vec3d(0,0,force_b[2])) - v_o* o_viscosity;
		Mat o_acc = F_o/o_mass;
		v_o += o_acc*dt;
		x_o += v_o*dt;

		x_a_old = x_a;
		x_b_old = x_b;
		x_o_old = x_o;
		for(int i=0; i<3; i++){
			X_a_old[i] = X_a[i];
			X_b_old[i] = X_b[i];
		}

		vector<double> features;
		
		features.push_back(X_a[2]);
		features.push_back(X_b[2]);
		features.push_back(force[2]);
		features.push_back(force_b[2]);
		batchTRainer.addFeatureVector(features);
		
		
		if(fLearningStatus == LS_PREDICTION){
			vector<vector<double>> predData = batchTRainer.predict(batchTRainer.getDataSet(), 10);
			if(!predData.empty()){
				static double fa = 0.0;
				static double fb = 0.0;

				vector<double> data = predData.back();
				double tau = 1.0;
			
				fa = (1.0-tau)*fa + tau*data[2];
				fb = (1.0-tau)*fb + tau*data[3];

				//double max_f = 20;
				//if(fa > max_f) fa = max_f;
				//if(fa < -max_f) fa = -max_f;

				//fa = (1.0-tau)*fa + tau*force[2];
				//fb = (1.0-tau)*fb + tau*force_b[2];

				double a[] = {0,0,fa};
				double b[] = {0,0,fb};

				//printf("%f, %f %f, %f \n", data[0], data[2], X_a[2], X_b[2]);
				//printf("%f, %f -> %f, %f \n", force[2], force_b[2], data[2], data[3]);
			
			
				hdlMakeCurrent(gHaptics_a.getDeviceHandle());
				hdlSetToolForce(a);
			//	hdlMakeCurrent(gHaptics_b.getDeviceHandle());
			//	hdlSetToolForce(b);
		
			}
		}else{
			hdlMakeCurrent(gHaptics_a.getDeviceHandle());
			force[2] = 10;
			hdlSetToolForce(force);
			//hdlMakeCurrent(gHaptics_b.getDeviceHandle());
			//hdlSetToolForce(force_b);
		}
		



		hapticdata_a.pop_front();
		hapticdata_b.pop_front();
	}
	
	

	oldtime = time;


    return HDL_SERVOOP_CONTINUE;

}
*/

//--------------------------------------------------------------
void testApp::setup(){
	
	batchTRainer.setModel(VARModel(6, 3));
	//batchTRainer.constructModelFromCoeffsFile("coef1.txt");
	
	fLearningStatus = LS_NONE;

	/*
	batchTRainer.loadFromDataFile("data.txt", 6);
	batchTRainer.train();
	batchTRainer.saveCoeffsAsFile("coef2.txt");
	int a = 0;
	
	return;
	*/

	fCapture  = false;

	ofBackground(0, 0, 0);
	ofSetVerticalSync(true);
	ofSetCircleResolution(80);
	ofEnableSmoothing();

	//hapticDevices.initHDL();
	
	
	

	//gHaptics_a.init("FALCON_1", gCubeEdgeLength, gStiffness);
	//gHaptics_b.init("FALCON_2", gCubeEdgeLength, gStiffness);
	
	//g_device.push_back(gHaptics_a.getDeviceHandle());
	//g_device.push_back(gHaptics_b.getDeviceHandle());

	//hdlStart();

	//const bool bBlocking = false;
	//g_touchOp = hdlCreateServoOp(touchScene,(void*)0, bBlocking);

	drawgl.bset();

	// Some time is required between init() and checking status,
	// for the device to initialize and stabilize.  In a complex
	// application, this time can be consumed in the initGL()
	// function.  Here, it is simulated with Sleep().
	Sleep(500);

	vector<int> id;
	id.push_back(0);
	hapticDevices.initialize(new HapticWorldModel(id), id);
	
	/*
	if(!hapticDevices.initialize(new HapticWorldModel(id), id, HAPTICMODE_VIRTUAL, 1)){
		printf("ÇÝÆ½ ÃÊ±âÈ­ ¸øÇÔ");
	}else{
		printf("ÇÝÆ½ ÃÊ±âÈ­ ¼º°ø");
	}
	*/
}


void testApp::update()
{
	aState ballstate = hapticDevices.getHapticWorldModel()->getBallState();
	aHapticDevice* hd = hapticDevices.findDevice(0);
	
	if(fLearningStatus == LS_CAPTURE){
		vector<double> features;
		//features.push_back(ballstate.xt[0]);
		//features.push_back(ballstate.xt[1]);
		//features.push_back(ballstate.xt[2]);
		features.push_back(hd->state.xt[0]);
		features.push_back(hd->state.xt[1]);
		features.push_back(hd->state.xt[2]);
		//features.push_back(hd->force[0]);
		//features.push_back(hd->force[1]);
		//features.push_back(hd->force[2]);
		batchTRainer.addFeatureVector(features);
	}
	
}

//--------------------------------------------------------------

void testApp::CheckHapticButtonInput()
{
	hapticDevices.synchFromServo();
	aHapticDevice* haptic = hapticDevices.findDevice(0);
	unsigned char btnstate = (unsigned char) haptic->button;

	//printf("%d, %d, %d, %d \n", haptic->button & 0x01,haptic->button & 0x02,haptic->button & 0x04,haptic->button & 0x08);
	static bool isBtnClicket = false;
	static int btn;

	if(btnstate == 0 && isBtnClicket == true){

		if(btn == 1){	
			hapticDevices.setThrowingCommand(0);
		}else if(btn == 2){
			hapticDevices.setThrowingCommand(1);
		}else if(btn == 4){
			hapticDevices.setThrowingCommand(2);
		}
		btn = 0;
		isBtnClicket = false;
	}

	if(btnstate != 0){
		isBtnClicket = true;
		btn = btnstate;
	}
}

void testApp::draw()
{
	CheckHapticButtonInput();
	/*
	static int cnt = 0;
	if(cnt++ % 100 == 0){
		aHapticDevice* haptic = hapticDevices.findDevice(0);
		hapticDevices.setThrowingCommand(1);
	}
	*/
//	printf("%f, %f, %f \n", ((double*)x_b.data)[0], ((double*)x_b.data)[1], ((double*)x_b.data)[2]);

//	x_a = rotMat(0.0, toRadian( 90.0), 0.0)*x_a + Mat(Vec3d(0.1,0,0));
//	x_b = rotMat(0.0, toRadian(-90.0), 0.0)*x_b - Mat(Vec3d(0.1,0,0));
	//x_b.at<double>(0,0) = -0.2;

	//printf("%f, %f, %f \n", ((double*)x_b.data)[0], ((double*)x_b.data)[1], ((double*)x_b.data)[2]);

	//drawgl.Canvas1(pos_c1, pos_c2, pos_b, 0.11 );//Client1_packet.val.pos, Client2_packet.val.pos, haptic.gBallPosition, haptic.gBallRadius);
	
	aHapticDevice* hd = hapticDevices.findDevice(0);
	aState ballstate = hapticDevices.getHapticWorldModel()->getBallState();
	drawgl.drawHapticScene(hd->state.xt, ballstate.xt);

	//drawgl.bdraw();
	//drawgl.bupdate();
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

	if(key == 'r'){
		x_o = Mat(Vec3d(0.05,0,0));
		x_o_old = x_o;
	}else if(key == 'o'){
		fLearningStatus = LS_ORIG;
	}else if(key == 'n'){
		fLearningStatus = LS_NONE;

	}else if(key == 'c'){

		//fCapture = true;
		fLearningStatus = LS_CAPTURE;
		batchTRainer.getDataSet().clear();

		printf("data will be captured \n");
		printf("learning status : capture \n");

	}else if(key == 't'){
		printf("learning status : training\n");
		fLearningStatus = LS_TRAINING;
		fCapture = false;
		batchTRainer.train();
		batchTRainer.saveCoeffsAsFile("coef1.txt");
		list<vector<double>> data = batchTRainer.getDataSet();
		
		FILE *fp;
		fp = fopen("data.txt", "w");
		for(list<vector<double>>::iterator it = data.begin(); it != data.end() ; ++it){
			for (int i=0; i< (*it).size(); i++){
				fprintf(fp,"%.20f, ", (*it)[i] );
			}
			fprintf(fp, "\n");
		}
		fclose(fp);
		
		printf("coeffs are computed now \n");
		fLearningStatus = LS_PREDICTION;
		printf("learning status : prediction\n");
	}

	
	/*
	if(key == 'l'){
		force[0] -= 1;
	}else if(key == 'r'){
		force[0] += 1;
	}else if(key == 'u'){
		force[1] += 1;
	}else if(key == 'd'){
		force[1] -= 1;
	}else if(key == 'f'){
		force[2] += 1;
	}else if(key == 'b'){
		force[2] -= 1;
	}else if(key == 'q'){
		kp += 50;
	}else if(key == 'a'){
		kp -= 50;
	}


	printf("%f, %f, %f \n", force[0], force[1], force[2]);
	*/
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

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