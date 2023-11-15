#include "testApp.h"
testApp* pTestApp = NULL;
void testApp::updateHapticInfo(HapticPacket packet)
{
	hpkt = packet;
}
bool __stdcall testApp::onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	if(nBufLen<=0)return false;
	unsigned char* packetData = (unsigned char*)pBuf;

	if(nChannel == HAPTIC_CHNNEL){
		HapticPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		//packet = pkt;
		//-------------------------------------------------------------------------------------------------------------------------------------
		//pTestApp->getHapticPacketQueue()->push(packet);
		pTestApp->updateHapticInfo(packet);

		//-------------------------------------------------------------------------------------------------------------------------------------
		//canvas(*packet);
		printf("%f, %f, %f \n", packet.val.sensors.fingerpos[0][0], packet.val.sensors.fingerpos[0][1], packet.val.sensors.fingerpos[0][2]);

		return true;
	}



	return false;
}

static int dataSendingThread(void* data)
{
	testApp* app = (testApp*)data;
	
	for(;;){
		/*
		if(app->mediaEncoder.getAudioBuffer()->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(app->mediaEncoder.getAudioBuffer(), &packet, 1, &time_stamp) < 0) {
				return -1;
			}
			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				app->getSocket()->SendData("MirrorReceiver", (char*)buffer, size, AUDIO_CHNNEL);
				//printf("audio %d, ", packet.size);	
				av_free(buffer);
			}
		}
		//Sleep(1);
		*/
		if(app->mediaEncoder.getVideoBuffer()->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(app->mediaEncoder.getVideoBuffer(), &packet, 1, &time_stamp) < 0) {
				return -1;
			}
			
			AVIOContext* avioContext = NULL;
			uint8_t* buffer = NULL;
			if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				// --------------------------------
				// Write data hear
				// --------------------------------
				avio_w8(avioContext, '#'); // Magic number
				avio_wb64(avioContext, time_stamp);
				avio_write(avioContext, packet.data, packet.size);
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				app->getSocket()->SendData("MirrorReceiver", (char*)buffer, size, VIDEO_CHNNEL);
				//printf("video %d \n", packet.size);
				//printf("%d => %lld \n", gVideoQueue->nb_packets, time_stamp);
				av_free(buffer);
			}
			
			if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {

				AVFrame* decodedFrame = app->mMediaDecoder.decodeVideoPacketToFrame(packet);
				if(decodedFrame != NULL){
					app->copyFrameDataToRgbBuffer(decodedFrame);
					av_free(decodedFrame);	
				}
			}
		}
	}
}


void testApp::copyFrameDataToRgbBuffer(AVFrame* decodedFrame)
{
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, 640, 480);
	//uint8_t *buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameRGB, mRgbBuffer, AV_PIX_FMT_RGB24, 640, 480);
	SwsContext* img_convert_ctx = sws_getContext(640, 480, AV_PIX_FMT_YUV420P, 640, 480, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, decodedFrame->data, decodedFrame->linesize, 0, 480, pFrameRGB->data, pFrameRGB->linesize); 
	av_free(pFrameRGB);
}

//--------------------------------------------------------------
void testApp::setup(){	 
	pTestApp = this;
	ofSetBackgroundAuto(false);
	ofEnableAlphaBlending();
	ofSetBackgroundColor(0,0,0);

	//char deviceName[] = "video=Logitech Webcam 905:audio=Microphone(Webcam 905)";
	char deviceName[] = "video=Logitech Webcam 905";
	if(!mediaEncoder.initialize(MediaStreamServer, deviceName, AV_CODEC_ID_MPEG1VIDEO, 640, 480, 1000000, 10, 0)){
		printf("codec initialization failure \n");
	}
	if(!mMediaDecoder.initialize(MediaStreamClient, NULL, AV_CODEC_ID_MPEG1VIDEO)){
		printf("codec initialization failure \n");
	}
	mediaEncoder.startMediaCapture();

	std::map<int, std::string> mDevice;
	mDevice[0] = "Logitech Camera";
	if(mNetSender.Init(SERVER_IP_ADDRESS,mDevice, "MirrorSender", "", onDataReceived)){
		printf("Server connection is successful.");
		mNetSender.AddReceiveChannel(VIDEO_CHNNEL);
		mNetSender.AddReceiveChannel(AUDIO_CHNNEL);
		mNetSender.AddReceiveChannel(HAPTIC_CHNNEL);
		mNetSender.startThread();
	}
	
	SDL_CreateThread(dataSendingThread, (void*)this);
	
	ofSetVerticalSync(true);	
	ofSetCircleResolution(80);
	ofBackground(0);
	//------------Leap-------------
	
	hexcolor[0] = 0xFF0000;
	hexcolor[1] = 0xFF5E00;
	hexcolor[2] = 0xFFFF48;
	hexcolor[3] = 0x1DDB16;
	hexcolor[4] = 0x0054FF;
	counter =0;
	
	//------------------------------

	int i =0;

	//soundStream.listDevices();
	//if you want to set a different device id soundStream.setDeviceID(0); //bear in mind the device id corresponds to all audio devices, including  input-only and output-only devices.
	// 0 output channels, 2 input channels, 44100 samples per second, 256 samples per buffer, 4 num buffers (latency)
	int bufferSize = 256;
	soundStream.setup(this, 1, 1, 44100, bufferSize, 4);
		
	mGlImageTexture.allocate(VIDEO_WIDTH, VIDEO_HEIGHT, GL_RGB);

	
}


void testApp::exit()
{
	mGlImageTexture.clear();

	soundStream.stop();
	soundStream.close();
			
	mNetSender.stopThread();
}

//--------------------------------------------------------------
void testApp::update(){
	
	// alloc rgb buffer to opengl texture
	mGlImageTexture.loadData((unsigned char*)(mRgbBuffer), WIDTH, HEIGHT, GL_RGB);
	counter+=0.033f;

}

//--------------------------------------------------------------

void drawAxis(float length)
{
	GLUquadricObj *gluQuad;
	gluQuad = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuad, GLU_FILL); 

	float radius = length*0.04f;
	float cone_radius = radius*3.0f;
	float cylinder_len = length*0.75f;
	float cone_len = length*0.25f;


	glPushMatrix();
	glRotatef( 90.0, 0.0, 1.0, 0.0 );
	glColor4f( 1.0, 0.0, 0.0, 1.0 );
	gluCylinder(gluQuad,radius, radius,cylinder_len,10,5); 
	glTranslatef(0.0f, 0.0f, cylinder_len);
	gluCylinder(gluQuad,cone_radius,0.0,cone_len,10,5); 
	glPopMatrix();

	glPushMatrix();
	glRotatef( -90.0, 1.0, 0.0, 0.0 );
	glColor4f( 0.0, 1.0, 0.0, 1.0 );
	gluCylinder(gluQuad,radius, radius,cylinder_len,10,5); 
	glTranslatef(0.0f, 0.0f, cylinder_len);
	gluCylinder(gluQuad,cone_radius,0.0,cone_len,10,5); 

	glPopMatrix();

	glPushMatrix();
	glRotatef( 0.0, 0.0, 0.0, 1.0 );
	glColor4f( 0.0, 0.0, 1.0, 1.0 );
	gluCylinder(gluQuad,radius, radius,cylinder_len,10,5); 
	glTranslatef(0.0f, 0.0f, cylinder_len);
	gluCylinder(gluQuad,cone_radius,0.0,cone_len,10,5); 

	glPopMatrix();
}

void setProperLight()
{
	static int  mat_f = 1;
	GLfloat     mat_amb_diff[]  = {0.9, 0.9, 0.0, 1.0};
	GLfloat     mat_specular[]  = {0.5, 0.5, 0.5, 1.0};
	GLfloat     mat_shininess[] = {3.0};
	GLfloat     light_ambient[] = { 0.01, 0.01, 0.01, 1.0 };
	GLfloat     light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat     light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat     light_position[] = { 100.0, 500.0, -700.0, 1.0 };

	if( mat_f ) {
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_amb_diff);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);	
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
		mat_f = 0;
	}

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

void testApp::drawViewportOutline(const ofRectangle & viewport){
	ofPushStyle();
	ofFill();
	ofSetColor(0);
	ofSetLineWidth(0);
	ofRect(viewport);
	ofNoFill();
	ofSetColor(25);
	ofSetLineWidth(1.0f);
	ofRect(viewport);
	ofPopStyle();
}

void testApp::DrawFingerPos(HapticPacket packet)
{
	viewport3D1.x = 640;
	viewport3D1.y = 0;
	viewport3D1.width = ofGetWidth()-640;
	viewport3D1.height = ofGetHeight();

	drawViewportOutline(viewport3D1);
	camera1.begin(viewport3D1);

	ofTranslate(0,-200,0);
	setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	//------------Sphere---------------
	ofPushMatrix();
	GLUquadricObj *gluQuadps;
	gluQuadps = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps, GLU_FILL); 
	float ndist;
	if(packet.val.effectors.dist1 > packet.val.effectors.dist2)
		ndist = packet.val.effectors.dist2;
	else									ndist = packet.val.effectors.dist1;
	if(ndist > SPHERE_SIZE)					glColor4f( 0.4, 0.4, 1.0, 1.0 );
	else									glColor4f( ndist/250, ndist/250, ndist/50, 1.0 );	
	ofTranslate(0.0f, SPHERE_JUMP ,0.0f );
	gluSphere( gluQuadps, SPHERE_SIZE, 200, 200 );
	ofPopMatrix();

	//-----Palm---------------------------------------	

	ofPushMatrix();
	ofTranslate(packet.val.sensors.palmpos[0], packet.val.sensors.palmpos[1], packet.val.sensors.palmpos[2]);
	drawAxis(30.0f);
	ofPopMatrix();
	
	
	//-----finger------
	for(int i=0; i<packet.val.sensors.fingercnt; i++)
	{
		ofPushMatrix();
		GLUquadricObj *gluQuad1;
		gluQuad1 = gluNewQuadric(); 
		gluQuadricDrawStyle(gluQuad1, GLU_FILL); 
		ofSetHexColor(hexcolor[i]);		
		ofTranslate(packet.val.sensors.fingerpos[i][0], 
			packet.val.sensors.fingerpos[i][1], 
			packet.val.sensors.fingerpos[i][2]);
		gluSphere( gluQuad1, FINGER_SPHERE_SIZE, 32, 32 );

		// 		glBegin(GL_LINES);
		// 		glColor4f( 0.0, 1.0, 1.0, 1.0 );
		// 
		// 		glVertex3f( 0, 0, 0 );
		// 		//glVertex3fv(packet.val.sensors.fingerdir[i].toFloatPointer());
		// 		glEnd();
		drawAxis(10.0f);
		ofPopMatrix();
	}
	
	ofDisableLighting();
	camera1.end();

}
void testApp::BounceCircle(HapticPacket packet)
{
	viewport3D4.x = 0;
	viewport3D4.y = 480;
	viewport3D4.width = 640;
	viewport3D4.height = ofGetHeight()-480;

	drawViewportOutline(viewport3D4);

	// ofPushView() / ofPopView() are automatic
	camera3.begin(viewport3D4);


	//----Light-----
	// 	setProperLight();
	// 	glEnable(GL_LIGHTING);
	// 	glEnable(GL_LIGHT0
	//----Camera Pos----
	//camera.setPosition(ofVec3f(50,50,-350));
	camera3.lookAt(ofVec3f(0.0f,0.0f,0.0f), ofVec3f(0.0f,1.0f,0.0f));

	//--------------------------- circles
	//let's draw a circle:
	if((packet.val.effectors.dist1 <SPHERE_SIZE)&&(packet.val.effectors.dist1>0))
	{

		ofPushMatrix();
		ofTranslate(-150,0,0);
		static float freq = 3.0f*packet.val.effectors.dist1/10;
		ofSetHexColor(hexcolor[0]);
		float radius = (SPHERE_SIZE-packet.val.effectors.dist1)/15 + 
			(SPHERE_SIZE-packet.val.effectors.dist1)*0.5 * sin(2.0*3.141592*freq*counter);
		ofFill();		// draw "filled shapes"
		ofCircle(10,60,radius);

		// now just an outline
		ofNoFill();
		ofSetHexColor(0xCCCCCC);
		ofCircle(10,60,70);
		//ofSetHexColor(0x000000);
		ofDrawBitmapString("Vibrator1", -20,-50);
		ofPopMatrix();

	}

	if((packet.val.effectors.dist2 <SPHERE_SIZE)&&(packet.val.effectors.dist2>0))
	{

		ofPushMatrix();
		ofTranslate(150,0,0);
		static float freq = 3.0f*packet.val.effectors.dist2/10;
		ofSetHexColor(hexcolor[1]);
		float radius = (SPHERE_SIZE-packet.val.effectors.dist2)/15 + 
			(SPHERE_SIZE-packet.val.effectors.dist2)*0.5 * sin(2.0*3.141592*freq*counter);
		ofFill();		// draw "filled shapes"
		ofCircle(10,60,radius);

		// now just an outline
		ofNoFill();
		ofSetHexColor(0xCCCCCC);
		ofCircle(10,60,70);
		//ofSetHexColor(0x000000);
		ofDrawBitmapString("Vibrator2", -20,-50);
		ofPopMatrix();

	}
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	camera3.end();

}
void testApp::draw(){
	
	ofSetColor(0,0,0,20);
	ofRect(0,0,ofGetWidth(),ofGetHeight());
	ofSetColor(255,255,255,50);
	mGlImageTexture.draw(0,0,VIDEO_WIDTH,VIDEO_HEIGHT);
	hpkt.val.effectors.dist1 = 
		sqrt((0.0f - hpkt.val.sensors.fingerpos[0][0]) * (0.0f - hpkt.val.sensors.fingerpos[0][0]) 
		+ (SPHERE_JUMP - hpkt.val.sensors.fingerpos[0][1]) * (SPHERE_JUMP - hpkt.val.sensors.fingerpos[0][1])
		+(0.0f - hpkt.val.sensors.fingerpos[0][2]) * (0.0f - hpkt.val.sensors.fingerpos[0][2]) );

	hpkt.val.effectors.dist2 = 
		sqrt((0.0f - hpkt.val.sensors.fingerpos[1][0]) * (0.0f - hpkt.val.sensors.fingerpos[1][0]) 
		+ (SPHERE_JUMP - hpkt.val.sensors.fingerpos[1][1]) * (SPHERE_JUMP - hpkt.val.sensors.fingerpos[1][1])
		+(0.0f - hpkt.val.sensors.fingerpos[1][2]) * (0.0f - hpkt.val.sensors.fingerpos[1][2]) );

	DrawFingerPos(hpkt);
	if((hpkt.val.effectors.dist1 <SPHERE_SIZE)||(hpkt.val.effectors.dist1 <SPHERE_SIZE))
	{
		if((hpkt.val.effectors.dist1 >0)||(hpkt.val.effectors.dist1 >0))
		this->getSocket()->SendData("MirrorReceiver", (char*)hpkt.udata, sizeof(HapticData), HAPTIC_CHNNEL);
	}

	BounceCircle(hpkt);


	
	

}


//--------------------------------------------------------------
void testApp::audioIn(float * input, int bufferSize, int nChannels){	

	AudioPacket audioPacket;

	for (int i = 0; i < bufferSize; i++){
		audioPacket.val.left[i] = input[i]*0.5;
	}
	mNetSender.SendData("MirrorReceiver", (char*)audioPacket.udata, sizeof(AudioData), AUDIO_CHNNEL);
}

//--------------------------------------------------------------
void testApp::audioOut(float * output, int bufferSize, int nChannels)
{
// 	for (int i = 0; i < bufferSize; i++){
// 		output[i*nChannels] = left[i];
// 		//output[i*nChannels + 1] = right[i];
// 	}
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 

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

