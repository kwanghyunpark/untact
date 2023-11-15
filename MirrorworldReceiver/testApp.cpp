#include "testApp.h"

testApp* pTestApp = NULL;
SwrContext* swr_ctx; 

//--------------------------------------------------------------
void testApp::setup(){	 
	
	pTestApp = this;

	if(!mediaDecoder.initialize(MediaStreamClient, NULL, AV_CODEC_ID_MPEG1VIDEO)){
		printf("codec initialization failure \n");
	}
	hav::packet_queue_init(&mVideoQueue); hav::packet_queue_start(&mVideoQueue);
	//hav::packet_queue_init(&mAudioQueue); hav::packet_queue_start(&mAudioQueue);

// 	swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLTP, 44100, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 44100,0, NULL);
// 	if (!swr_ctx || swr_init(swr_ctx) < 0) {
// 		printf(" swr context initialization failure\n");
// 	}
	
	std::map<int, std::string> mDevice;
	if(mNetSender.Init("114.70.63.186",mDevice, "MirrorReceiver", "", onDataReceived)){
		printf("Server connection is successful.");
		mNetSender.AddReceiveChannel(VIDEO_CHNNEL);
		mNetSender.AddReceiveChannel(AUDIO_CHNNEL);
		mNetSender.AddReceiveChannel(HAPTIC_CHNNEL);
		mNetSender.startThread();
	}

	ofSetVerticalSync(true);
	ofSetCircleResolution(80);
	

	// Set sound output device
	int bufferSize = 256;
	soundStream.setup(this, 1, 0, 44100, bufferSize, 4);
	
	mGlImageTexture.allocate(WIDTH,HEIGHT,GL_RGB);



	controller.addListener(listener);


	Inside_cnt1 =0;
	Inside_cnt2 =0;
	oldpos1 =0;
	oldpos2 =0;
	npos1=0;
	npos2=0;
	serial_delay_cnt1 = SERIAL_WAIT;
	serial_delay_cnt2 = SERIAL_WAIT;
	sound_cnt = SOUND_WAIT;
	if(synth.loadSound("synth.wav"))
		exit();
	synth.setVolume(0.75f);

	
	hexcolor[0] = 0xFF0000;
	hexcolor[1] = 0xFF5E00;
	hexcolor[2] = 0xFFFF48;
	hexcolor[3] = 0x1DDB16;
	hexcolor[4] = 0x0054FF;
	counter =0;

	if(serial.setup("COM6", 9600))
	{
		printf("Serial Connect OK~!\n");

	} 
	//serial.startContinuousRead();
	ofAddListener(serial.NEW_MESSAGE,this,&testApp::onNewMessage);
}
void testApp::onNewMessage(string & message)
{
	cout << "onNewMessage, message: " << message << "\n";
	if(message.length() <3)
		serial_f = TRUE;
	else
		serial_f = FALSE;



}
void testApp::exit()
{
	mGlImageTexture.clear();
	
	soundStream.stop();
	soundStream.close();
	
	mNetSender.stopThread();
}

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
		//printf("Re : %f, %f, %f \n", packet.val.sensors.fingerpos[0][0], packet.val.sensors.fingerpos[0][1], packet.val.sensors.fingerpos[0][2]);
		printf("Re : %f, %f\n", packet.val.effectors.dist1,packet.val.effectors.dist2);

		return true;
	}else if(nChannel == AUDIO_CHNNEL){
		AudioPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->getAudioPacketQueue()->push(packet);

		/*
		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		char head = avio_r8(avioContext);
		if(head == '#'){
			int64_t time_stamp = avio_rb64(avioContext);
			// -------------------------------
			//	printf("%c : %lld (%d) \n", head, time_stamp, nBufLen);
			AVPacket encoded_packet;
			av_init_packet(&encoded_packet);
			encoded_packet.data = (uint8_t*)packetData + (sizeof(char) + sizeof(int64_t));
			encoded_packet.size = nBufLen - (sizeof(char) + sizeof(int64_t));
			hav::packet_queue_put(&pTestApp->mAudioQueue, &encoded_packet, time_stamp);

		}
		av_free(avioContext);
		*/
		return true;

	}else if(nChannel == VIDEO_CHNNEL){
		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);

		char head = avio_r8(avioContext);
		if(head == '#'){
			int64_t time_stamp = avio_rb64(avioContext);
			// -------------------------------
			AVPacket encoded_packet;
			av_init_packet(&encoded_packet);
			encoded_packet.data = (uint8_t*)packetData + (sizeof(char) + sizeof(int64_t));
			encoded_packet.size = nBufLen - (sizeof(char) + sizeof(int64_t));
			hav::packet_queue_put(&pTestApp->mVideoQueue, &encoded_packet, time_stamp);
		}
		//printf("%d \n",pTestApp->mVideoQueue.nb_packets );
		av_free(avioContext);
		return true;
	}
return false;
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
	GLfloat     mat_shininess[] = {5.0};
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


void testApp::Canvas3(Ldata &ldata)
{
	viewport3D3.x = 0;
	viewport3D3.y = 480;
	viewport3D3.width = 640;
	viewport3D3.height = ofGetHeight()-480;

	drawViewportOutline(viewport3D3);

	// ofPushView() / ofPopView() are automatic
	camera3.begin(viewport3D3);


	//----Light-----
	// 	setProperLight();
	// 	glEnable(GL_LIGHTING);
	// 	glEnable(GL_LIGHT0
	//----Camera Pos----
	//camera.setPosition(ofVec3f(50,50,-350));
	camera3.lookAt(ofVec3f(0.0f,0.0f,0.0f), ofVec3f(0.0f,1.0f,0.0f));

	//--------------------------- circles
	//let's draw a circle:
	if(ldata.dist[0] <SPHERE_SIZE)
	{

		ofPushMatrix();
		ofTranslate(-150,0,0);
		static float freq = 3.0f*ldata.dist[0]/10;
		ofSetHexColor(hexcolor[0]);
		float radius = (SPHERE_SIZE-ldata.dist[0])/15 + (SPHERE_SIZE-ldata.dist[0])*0.5 * sin(2.0*3.141592*freq*counter);
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

	if(ldata.dist[1] <SPHERE_SIZE)
	{

		ofPushMatrix();
		ofRotateX(ldata.fdir->x);
		ofRotateY(ldata.fdir->y);
		ofRotateZ(ldata.fdir->z);
		ofTranslate(150,0,0);
		static float freq = 3.0f*ldata.dist[1]/10;
		ofSetHexColor(hexcolor[1]);
		float radius = (SPHERE_SIZE-ldata.dist[1])/15 + (SPHERE_SIZE-ldata.dist[1])*0.5 * sin(2.0*3.141592*freq*counter);
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

void testApp::SerialSend(int id, float dist)
{
	if (id == 1)
	{
		if((dist <= SPHERE_SIZE)&&(dist >0))
		{
			npos1 = 8-(abs(dist)/(SPHERE_SIZE/8))+1;
			printf("npos : %d, oldpos : %d\t %f\n", npos1, oldpos1,dist);
			if ((oldpos1 != npos1))
			{
				printf("IN\t");
				{
					char indata[100]={0};
					sprintf(indata, "!%d%d\r",npos1,npos2);
					//printf("Serial_Delay_cnt! %d\n", serial_delay_cnt);
					if(serial_delay_cnt1 == SERIAL_WAIT)
					{
						serial.writeString(indata);
						serial_delay_cnt1--;
						oldpos1 = npos1;
					}

					serial_delay_cnt1--;

					if (serial_delay_cnt1 == 0)
						serial_delay_cnt1 = SERIAL_WAIT;



				}
			}		
		}
		else
		{
			char indata[100]={0};
			sprintf(indata, "!%d%d\r",0,npos2);
			//printf("Serial_Delay_cnt! %d\n", serial_delay_cnt);
			if(serial_delay_cnt1 == SERIAL_WAIT)
			{
				serial.writeString(indata);
				serial_delay_cnt1--;
				oldpos1 = npos1;
			}

			serial_delay_cnt1--;

			if (serial_delay_cnt1 == 0)
				serial_delay_cnt1 = SERIAL_WAIT;



		}

	}
	else
	{
		if((dist <= SPHERE_SIZE)&&(dist >0))
		{
			npos2 = 8-(abs(dist)/(SPHERE_SIZE/8))+1;
			printf("npos : %d, oldpos : %d\t %f\n", npos2, oldpos2,dist);
			if ((oldpos2 != npos2))
			{
				printf("IN\t");
				{
					char indata[100]={0};
					sprintf(indata, "!%d%d\r",npos1,npos2);
					//printf("Serial_Delay_cnt! %d\n", serial_delay_cnt);
					if(serial_delay_cnt2 == SERIAL_WAIT)
					{
						serial.writeString(indata);
						serial_delay_cnt2--;
						oldpos2 = npos2;
					}

					serial_delay_cnt2--;

					if (serial_delay_cnt2 == 0)
						serial_delay_cnt2 = SERIAL_WAIT;



				}
			}		
		}
		else
		{
			char indata[100]={0};
			sprintf(indata, "!%d%d\r",npos1,0);
			//printf("Serial_Delay_cnt! %d\n", serial_delay_cnt);
			if(serial_delay_cnt2 == SERIAL_WAIT)
			{
				serial.writeString(indata);
				serial_delay_cnt2--;
				oldpos2 = npos2;
			}

			serial_delay_cnt2--;

			if (serial_delay_cnt2 == 0)
				serial_delay_cnt2 = SERIAL_WAIT;



		}
	}
}
void testApp::copyFrameDataToRgbBuffer(AVFrame* decodedFrame)
{
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, VIDEO_WIDTH, VIDEO_HEIGHT);
	//uint8_t *buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameRGB, mRgbBuffer, AV_PIX_FMT_RGB24, VIDEO_WIDTH, VIDEO_HEIGHT);
	SwsContext* img_convert_ctx = sws_getContext(VIDEO_WIDTH, VIDEO_HEIGHT,AV_PIX_FMT_YUV420P, VIDEO_WIDTH, VIDEO_HEIGHT, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, decodedFrame->data, decodedFrame->linesize, 0, VIDEO_HEIGHT, pFrameRGB->data, pFrameRGB->linesize); 
	av_free(img_convert_ctx);
	av_free(pFrameRGB);
}

//--------------------------------------------------------------
void testApp::update(){
	
	//printf("%d \n", mVideoQueue.nb_packets);
	if(mVideoQueue.nb_packets > 0){
		AVPacket packet;
		int64_t time_stamp;
		if(hav::packet_queue_get(&mVideoQueue, &packet, 1, &time_stamp) >= 0) {
			AVFrame* decodedFrame = mediaDecoder.decodeVideoPacketToFrame(packet);
			if(decodedFrame != NULL){

				copyFrameDataToRgbBuffer(decodedFrame);
				mGlImageTexture.loadData((unsigned char*)(mRgbBuffer), WIDTH, HEIGHT, GL_RGB);

				av_free(decodedFrame);	
			}
		}
	}
	ofSoundUpdate();
	counter = counter + 0.033f;
}

void testApp::Canvas1(Ldata &ldata)
{
	viewport3D3.x = 640;
	viewport3D3.y = 0;
	viewport3D3.width = ofGetWidth()-640;
	viewport3D3.height = ofGetHeight();

	drawViewportOutline(viewport3D3);

	// ofPushView() / ofPopView() are automatic
	camera2.begin(viewport3D3);

	
	//----Light-----
	setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//----Camera Pos----
	//camera2.setPosition(ofVec3f(50,50,-350));
	camera2.lookAt(ofVec3f(0.0f,0.0f,0.0f), ofVec3f(0.0f,1.0f,0.0f));
	ofTranslate(0,-200,0);
	ofDisableLighting();
	DrawParticle(hpkt);
	ofEnableLighting();
	//------sphere--------------------------------
// 	ofPushMatrix();
// 	GLUquadricObj *gluQuadps;
// 	gluQuadps = gluNewQuadric(); 
// 	gluQuadricDrawStyle(gluQuadps, GLU_FILL); 
// 	float ndist;
// 	if(ldata.dist[0] > ldata.dist[1])		ndist = ldata.dist[1];
// 	else									ndist = ldata.dist[0];
// 	if(ndist > SPHERE_SIZE)					glColor4f( 0.4, 0.4, 1.0, 1.0 );
// 	else									glColor4f( ndist/250, ndist/250, ndist/50, 1.0 );	
// 	ofTranslate(0.0f, SPHERE_JUMP ,0.0f );
// 	gluSphere( gluQuadps, SPHERE_SIZE, 200, 200 );
// 	ofPopMatrix();

	//-----Palm---------------------------------------	

	ofPushMatrix();
	ofTranslate(ldata.ppos.x, ldata.ppos.y, ldata.ppos.z);
	drawAxis(30.0f);
	ofPopMatrix();

	//------LeapMotion Finger Position1-------------------


	for(int i=0; i<ldata.fcnt; i++)
	{
		ofPushMatrix();
		GLUquadricObj *gluQuad1;
		gluQuad1 = gluNewQuadric(); 
		gluQuadricDrawStyle(gluQuad1, GLU_FILL); 
		//lColor4f( 1.0, 1.0, 0.0, 1.0 );
		ofSetHexColor(hexcolor[i]);
		ofTranslate(ldata.fpos[i].x, (ldata.fpos[i].y), (ldata.fpos[i].z));
		gluSphere( gluQuad1, FINGER_SPHERE_SIZE, 32, 32 );

		glBegin(GL_LINES);
		glColor4f( 1.0, 1.0, 1.0, 1.0 );
		glVertex3f( 0, 0, 0 );
		glVertex3fv( ldata.fdir[i].toFloatPointer());
		glEnd();

		drawAxis(10.0f);

		ofPopMatrix();
	}


	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);


	//--
	
	camera2.end();
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
void testApp::DrawParticle(HapticPacket packet)
{
	if(packet.val.effectors.dist1 > 0)
	{
		if(packet.val.effectors.dist1 <=SPHERE_SIZE)
		{
			InsideVec1[Inside_cnt1][0]= packet.val.sensors.fingerpos[0][0];
			InsideVec1[Inside_cnt1][1]= packet.val.sensors.fingerpos[0][1];
			InsideVec1[Inside_cnt1][2]= packet.val.sensors.fingerpos[0][2];
			Inside_cnt1++;
		}
	}
	if(packet.val.effectors.dist2 > 0)
	{
		if(packet.val.effectors.dist2 <=SPHERE_SIZE)
		{
			InsideVec1[Inside_cnt1][0]= packet.val.sensors.fingerpos[1][0];
			InsideVec1[Inside_cnt1][1]= packet.val.sensors.fingerpos[1][1];
			InsideVec1[Inside_cnt1][2]= packet.val.sensors.fingerpos[1][2];
			Inside_cnt1++;
		}
	}
	for(int i=0; i<Inside_cnt1; i++)
	{

		ofPushMatrix();
		GLUquadricObj *gluQuad;
		gluQuad = gluNewQuadric(); 
		gluQuadricDrawStyle(gluQuad, GLU_FILL); 
		float redist1=sqrt((0.0f - InsideVec1[i][0]) * (0.0f - InsideVec1[i][0]) 
			+ (SPHERE_JUMP - InsideVec1[i][1]) * (SPHERE_JUMP - InsideVec1[i][1])
			+(0.0f - InsideVec1[i][2]) * (0.0f - InsideVec1[i][2]) );

		glColor4f(redist1/250, redist1/250, redist1/50, 1.0 );
		ofTranslate(InsideVec1[i][0], InsideVec1[i][1], InsideVec1[i][2]);
		gluSphere(gluQuad, FINGER_SPHERE_SIZE,10.0f,10.0f);
		ofPopMatrix();
	}

}
//--------------------------------------------------------------
void testApp::draw()
{
	ofSetColor(0,0,0,20);
	ofRect(0,0,ofGetWidth(),ofGetHeight());
	ofSetColor(255,255,255,50);
	mGlImageTexture.draw(0,0,WIDTH,HEIGHT);
	
	HapticPacket hapticpacket; 
	ldata = listener.GetHandData();
	if(ldata->dist[0]<0.0f)
		ldata->dist[0]=1000.0f;
	if(ldata->dist[1]<0.0f)
		ldata->dist[1]=1000.0f;
	
	
	

	if(ldata->fcnt>0)
	{
		//Canvas3(*ldata);

		if(ldata->fcnt)
		{
			for(int i=0; i<ldata->fcnt; i++)
			{
				hapticpacket.val.sensors.fingerpos[i][0] = ldata->fpos[i].x;
				hapticpacket.val.sensors.fingerpos[i][1] = ldata->fpos[i].y;
				hapticpacket.val.sensors.fingerpos[i][2] = ldata->fpos[i].z;

				hapticpacket.val.sensors.fingerdir[i][0] = ldata->fdir[i].x;
				hapticpacket.val.sensors.fingerdir[i][1] = ldata->fdir[i].y;
				hapticpacket.val.sensors.fingerdir[i][2] = ldata->fdir[i].z;

				//hapticpacket.val.sensors.fingerdist[i] = ldata->dist[i];
			}
			hapticpacket.val.sensors.palmpos[0] = ldata->ppos.x;
			hapticpacket.val.sensors.palmpos[1] = ldata->ppos.y;
			hapticpacket.val.sensors.palmpos[2] = ldata->ppos.z;

			hapticpacket.val.sensors.palmdir[0] = ldata->pdir.x;
			hapticpacket.val.sensors.palmdir[1] = ldata->pdir.y;
			hapticpacket.val.sensors.palmdir[2] = ldata->pdir.z;

			hapticpacket.val.sensors.fingercnt = ldata->fcnt;

			// 	hapticpacket.val.sensors.x1 = ldata->fpos[0].x;
			// 	hapticpacket.val.sensors.x2 = ldata->fpos[0].y;
			// 	hapticpacket.val.sensors.x3 = ldata->fpos[0].z;


			

			
			//this->getSocket()->SendData("MirrorReceiver", (char*)hapticpacket.udata, sizeof(HapticData), HAPTIC_CHNNEL);
		}
		mNetSender.SendData("MirrorSender", (char*)hapticpacket.udata, sizeof(HapticData), HAPTIC_CHNNEL);
	}
	
	Canvas1(*ldata);
	BounceCircle(hpkt);

	if(hpkt.val.sensors.fingercnt >0)
	{
		SerialSend(2,hpkt.val.effectors.dist1);
		SerialSend(1,hpkt.val.effectors.dist2);
	}
	else
	{
		SerialSend(1, 10000);
		SerialSend(2, 10000);
	}

	

	// draw camera image received from server
	
	
	

	
}

//--------------------------------------------------------------
//void testApp::audioIn(float * input, int bufferSize, int nChannels)
//{	
//}

//--------------------------------------------------------------
void testApp::audioOut(float * output, int bufferSize, int nChannels){

	
	if(mAudioPacketQueue.size() < 1) return;

	AudioPacket packet = mAudioPacketQueue.front();
	for (int i = 0; i < bufferSize; i++){
		output[i*nChannels] = packet.val.left[i];
	}
	mAudioPacketQueue.pop();

	/*
	if(mAudioQueue.nb_packets > 0){
		AVPacket packet;
		int64_t time_stamp;
		if(hav::packet_queue_get(&mAudioQueue, &packet, 1, &time_stamp) >= 0) {
			//if(packet.stream_index == AVMEDIA_TYPE_AUDIO) {
				int got_frame = 0;
				// Decoder
				AVCodec  *audioCodec = avcodec_find_decoder(AV_CODEC_ID_FIRST_AUDIO);
				AVCodecContext* aCodecCtx = avcodec_alloc_context3(audioCodec);
				aCodecCtx->sample_rate = 44100;
				aCodecCtx->bit_rate = 1411200;
				aCodecCtx->channels = 1;
				aCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
				if(avcodec_open2(aCodecCtx, audioCodec, NULL) < 0){
					fprintf(stderr, "fail to open decoder \n");
				}
				AVFrame aFrame;
				int len1 = avcodec_decode_audio4(aCodecCtx, &aFrame, &got_frame, &packet);
				if(got_frame){
					const uint8_t **in = (const uint8_t **)aFrame.extended_data;
					uint8_t **out = &audio_buf1;
					int wanted_nb_samples = 256;
					int out_count = (int64_t)wanted_nb_samples;// + 256;
					int out_size  = av_samples_get_buffer_size(NULL, 1, out_count, AV_SAMPLE_FMT_FLTP, 0);
// 					//if (wanted_nb_samples != frame.nb_samples) {
// 						if (swr_set_compensation(swr_ctx, (wanted_nb_samples - aFrame.nb_samples) * 44100 / aFrame.sample_rate,
// 							wanted_nb_samples * 44100 / aFrame.sample_rate) < 0) {
// 								av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
// 						//		break;
// 						}
// 					//}
					unsigned int audio_buf1_size = 0;
					av_fast_malloc(&audio_buf1, &audio_buf1_size, out_size);

					int len2 = swr_convert(swr_ctx, out, out_count, in, aFrame.nb_samples);
					if (len2 >= 0) {
						//int resampled_data_size = len2 * 1 * av_get_bytes_per_sample(audio_tgt.fmt);
						int resampled_data_size = len2 * 1 * 4;
					
						for(int i=0; i< len2; i++){
							float a = *((float*)&audio_buf1[i*4]);
							left[i] = a;
						}
						//memcpy(audio_buf, audio_buf1, resampled_data_size);
					}
				}
			//}
		}
	}
	for (int i = 0; i < bufferSize; i++){
		output[i*nChannels] = left[i];
		//output[i*nChannels + 1] = right[i];
	}
	*/
}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	if(key == 'f'){
		//hav::packet_queue_flush(&mAudioQueue);
		while(mAudioPacketQueue.size() > 0){
			mAudioPacketQueue.pop();
		}
		while(mHapticPacketQueue.size() > 0){
			mHapticPacketQueue.pop();
		}
hav:
		hav::packet_queue_flush(&mVideoQueue);
	}
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

