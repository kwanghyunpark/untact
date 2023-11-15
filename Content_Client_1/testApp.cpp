#include "testApp.h"

testApp* pTestApp = NULL;
SwrContext* swr_ctx; 
AudioParams audio_tgt;
AudioParams audio_src;

uint8_t *audio_buf1;
unsigned int audio_buf1_size;
AVPacket pkt;
int audioreceivecnt;
int videoreceivecnt;
int hapticreceivecnt;
//--------------------------------------------------------------

#define VIDEOBUFFERSIZE 30

//#define VIDEO_AUDIO_SYNC
//#define VIDEO_HAPTIC_SYNC
//#define HAPTIC_AUDIO_SYNC

//#define NOT_SYNC

//====By KHH ====

void testApp::setTimerEventScheduler()
{
	int desired_ms = 4; // for haptic
	
	// 아래함수를 사용하려면 project/ settings | c/c++ | preprocessor definitions 에 "_WIN32_WINNT=0x0500"을 넣어주어야 함
	BOOL success = CreateTimerQueueTimer(&handleHapticDataSerder, NULL, (WAITORTIMERCALLBACK)_intraQueueTimerEventListenerForHapticDataSender,this, 0, desired_ms, WT_EXECUTEINTIMERTHREAD);
}

void testApp::killTimerEventScheduler()
{
	BOOL res = DeleteTimerQueueTimer(NULL, handleHapticDataSerder, NULL);

}

void testApp::queueTimerEventListenerForHapticDataSender()
{

	if(hapticDevices.getHapticQueueSender()->nb_packets > 0){
		hav::HapticData hapticdata;
		int64_t time_stamp;
		if(hav::haptic_queue_get(hapticDevices.getHapticQueueSender(), &hapticdata, 1, &time_stamp) < 0) {
			return;
		}
		hav::HapticPacket packet;
		packet.val = hapticdata;
		mNetSender.SendData("Content_Servers", (char*)packet.udata, sizeof(hav::HapticData), CLIENT_1_TO_SERVER_HAPTIC_CHANNEL);
	}
	
}


void testApp::drawHapticScene(double* pos1, double* ballState)
{
	// unit : meter

	Rect1.x = 0;
	Rect1.y = 0;
	Rect1.width = 640;
	Rect1.height = 480;

	double scale = Rect1.width/1.0; // We scale 1.0 meter to 640

	drawViewportOutline(Rect1);

	// ofPushView() / ofPopView() are automatic
	camera1.begin(Rect1);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	//----Light-----
	//setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	
	//----Camera Pos----
	//camera1.setPosition(ofVec3f(3.0,0.5,0.0)*scale);  // distance between camera and target object is 80cm
	//camera1.lookAt(ofVec3f(0.0f,-0.0f,-2.0f)*scale, ofVec3f(0.0f,1.0f,0.0f));

	camera1.setPosition(ofVec3f(0.0,0.7,1.2)*scale);  // distance between camera and target object is 80cm
	camera1.lookAt(ofVec3f(0.0f,-0.3f,-5.0f)*scale, ofVec3f(0.0f,1.0f,0.0f));
	//ofTranslate(0,0,0);
	ofDisableLighting();
	ofEnableLighting();

	// draw floor
	ofPushMatrix();
	GLUquadricObj *floorObj = gluNewQuadric(); 
	gluQuadricDrawStyle(floorObj, GLU_FILL); 
	ofTranslate(0*scale, -1.0*scale, -2.5*scale);
	ofSetHexColor(0x00A040);
	glScalef(200,2,500);
	glutSolidCube(0.01*scale);
	ofPopMatrix();


	// draw shutter
	ofPushMatrix();
	GLUquadricObj *shutterObj = gluNewQuadric(); 
	gluQuadricDrawStyle(shutterObj, GLU_FILL); 
	ofTranslate(0*scale, -0.5*scale, -5.0*scale);
	ofSetHexColor(0xFFFF00);
	glScalef(10,100,10);
	glutSolidCube(0.01*scale);
	ofPopMatrix();



	// drawing the right-hand side falcon
	ofPushMatrix();
	GLUquadricObj *gluQuadps1;
	gluQuadps1 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps1, GLU_FILL); 

	ofTranslate(pos1[0]*scale, pos1[1]*scale, pos1[2]*scale);
	//printf("%f, %f, %f \n",pos1[0]*scale, pos1[1]*scale, pos1[2]*scale);

	ofSetHexColor(0xFF0000);
	gluSphere( gluQuadps1, 0.05*scale, 20, 20 ); // radius of sphere size is 2cm 
	ofPopMatrix();

	
	// drawing the left-hand side falcon
	ofPushMatrix();
	GLUquadricObj *gluQuadps2;
	gluQuadps2 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps2, GLU_FILL); 
	ofTranslate(ballState[0]*scale, ballState[1]*scale, ballState[2]*scale);
	ofSetHexColor(0x0000FF);
	gluSphere( gluQuadps2, 0.05*scale, 20, 20 );
	ofPopMatrix();

	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	glDisable(GL_DEPTH_TEST);


	camera1.end();
}

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
// --------------





//싱크 및 버퍼 사이즈 조절
static int Sync(void* data)
{
	testApp* app = (testApp*)data;
	for(;;)
	{

		if(app->buff_flag == 1)
		{

		
		//Video & Audio Sync
		//--------------------------------------------------------------------------------------------------				
			AVPacket vpacket;
			AVPacket priorpacket;
			AVPacket newpacket;
			AVPacket nullpacket;
			av_init_packet(&nullpacket);
			int64_t prior_time_stamp;
			int64_t new_time_stamp=0;
			int64_t prior_err_time=0;
			int64_t new_err_time=0;
			int rePacketCnt=0;
			//printf("%f\n", (double)vtime_stamp);
			int64_t vtime_stamp=0;


			if(app->mVideoQueue.nb_packets > VIDEOBUFFERSIZE)
			{
				if(hav::packet_queue_get(&app->mVideoQueue, &vpacket,1,&vtime_stamp)>=0)
				{		

					if(hav::packet_queue_get(&app->mAudioQueue, &priorpacket,1,&prior_time_stamp)>=0)
					{
					
						for(;;)
						{
							prior_err_time = vtime_stamp - prior_time_stamp;
							
							if(app->mAudioQueue.nb_packets >0)
								new_time_stamp = app->mAudioQueue.first_pkt->time_stamp;
							new_err_time = vtime_stamp - new_time_stamp;
 							//printf("new_time_stamp2 : %lld\n", new_time_stamp); 						
						
							if((new_err_time<0)||(app->mAudioQueue.nb_packets ==0))
							{
								
								hav::packet_queue_put(&app->mSyncVideoQueue, &vpacket, vtime_stamp);
								hav::packet_queue_put(&app->mSyncAudioQueue, &priorpacket, prior_time_stamp);
								
								//app->sync_error_time = prior_err_time;
								//fprintf(app->errfile, "%0.3f\n", (double)app->sync_error_time);
								break;
							}
							else if(prior_err_time>=new_err_time)
							{
								rePacketCnt++;
								hav::packet_queue_get(&app->mAudioQueue, &priorpacket,1,&prior_time_stamp);
				// 							prior_time_stamp = new_time_stamp;
				// 							av_copy_packet(&priorpacket,&newpacket);
							}
						}
						//hav::packet_queue_put(&app->mSyncAudioQueue, &nullpacket, 0);
					}
				}	
			}
			//printf("remove packet : %d\n", rePacketCnt);
		}	

	//--------------------------------------------------------------------------------------------------

#ifdef VIDEO_HAPTIC_SYNC

		//Video & Haptic Sync
	//--------------------------------------------------------------------------------------------------
	
			
		AVPacket vpacket;
		hav::HapticData priorpacket;
		hav::HapticData newpacket;
		hav::HapticData nullpacket;
		//av_init_packet(&nullpacket);
		int64_t prior_time_stamp;
		int64_t new_time_stamp=0;
		int64_t prior_err_time=0;
		int64_t new_err_time=0;
		int rePacketCnt=0;
		int64_t vtime_stamp=0;


		if(hav::packet_queue_get(&app->mVideoQueue, &vpacket,1,&vtime_stamp)>=0)
		{

			if(hav::haptic_queue_get(&app->mHapticQueue, &priorpacket,1,&prior_time_stamp)>=0)
			{
					
				//printf("[in]nb_packet : %d\n", app->mHapticQueue.nb_packets);
				for(;;)
				{
					prior_err_time = vtime_stamp - prior_time_stamp;

					if(app->mHapticQueue.nb_packets >0)
						new_time_stamp = app->mHapticQueue.first_pkt->time_stamp;
					
					new_err_time = vtime_stamp - new_time_stamp;
					if((new_err_time<0)||(app->mHapticQueue.nb_packets ==0))
					{
						hav::packet_queue_put(&app->mSyncVideoQueue, &vpacket, vtime_stamp);
						hav::haptic_queue_put(&app->mSyncHapticQueue, &priorpacket, prior_time_stamp);

						app->sync_error_time = prior_err_time;
				
						fprintf(app->errfile, "%0.3f\n", (double)app->sync_error_time);
						break;
					}
					else if(prior_err_time>=new_err_time)
					{
						rePacketCnt++;
						hav::haptic_queue_get(&app->mHapticQueue, &priorpacket,1,&prior_time_stamp);
		// 							prior_time_stamp = new_time_stamp;
		// 							av_copy_packet(&priorpacket,&newpacket);
					}
				}
				//hav::packet_queue_put(&mSyncAudioQueue, &nullpacket, 0);
			}else{
				printf("AA\n");
			}
		}
#endif

#ifdef HAPTIC_AUDIO_SYNC
		//Video & Haptic Sync
		//--------------------------------------------------------------------------------------------------


		hav::HapticData vpacket;
		AVPacket priorpacket;
		AVPacket newpacket;
		AVPacket nullpacket;
		//av_init_packet(&nullpacket);
		int64_t prior_time_stamp;
		int64_t new_time_stamp=0;
		int64_t prior_err_time=0;
		int64_t new_err_time=0;
		int rePacketCnt=0;
		int64_t vtime_stamp=0;


		if(hav::haptic_queue_get(&app->mHapticQueue, &vpacket,1,&vtime_stamp)>=0)
		{

			if(hav::packet_queue_get(&app->mAudioQueue, &priorpacket,1,&prior_time_stamp)>=0)
			{
				for(;;)
				{
					prior_err_time = vtime_stamp - prior_time_stamp;

					if(app->mAudioQueue.nb_packets >0)
						new_time_stamp = app->mAudioQueue.first_pkt->time_stamp;
					new_err_time = vtime_stamp - new_time_stamp;
					if((new_err_time<0)||(app->mAudioQueue.nb_packets ==0))
					{
						hav::haptic_queue_put(&app->mSyncHapticQueue, &vpacket, vtime_stamp);
						hav::packet_queue_put(&app->mSyncAudioQueue, &priorpacket, prior_time_stamp);
						

						app->sync_error_time = prior_err_time;

						fprintf(app->errfile, "%0.3f\n", (double)app->sync_error_time);
						break;
					}
					else if(prior_err_time>=new_err_time)
					{
						rePacketCnt++;
						hav::packet_queue_get(&app->mAudioQueue, &priorpacket,1,&prior_time_stamp);
						// 							prior_time_stamp = new_time_stamp;
						// 							av_copy_packet(&priorpacket,&newpacket);
					}
				}
				//hav::packet_queue_put(&mSyncAudioQueue, &nullpacket, 0);
			}
		}

#endif
	//--------------------------------------------------------------------------------------------------				
	

		if(app->buff_flag ==0)
		{

	
			hav::HapticData hpacket;
			AVPacket vpacket;
			AVPacket apacket;
			int64_t vtimestamp;
			int64_t htimestamp;
			int64_t atimestamp;

			hav::packet_queue_get(&app->mVideoQueue, &vpacket, 1, &vtimestamp );
			hav::packet_queue_get(&app->mAudioQueue, &apacket, 1, &atimestamp );
			//hav::haptic_queue_get(&app->mHapticQueue, &hpacket, 1, &htimestamp );

			hav::packet_queue_put(&app->mSyncVideoQueue, &vpacket, vtimestamp );
			hav::packet_queue_put(&app->mSyncAudioQueue, &apacket, atimestamp );
			//hav::haptic_queue_put(&app->mSyncHapticQueue, &hpacket, htimestamp );
		}

	}
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

void testApp::drawAxis(float length)
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
	GLfloat     light_position[] = { 100.0, 500.0, 700.0, 1.0 };

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
void testApp::Canvas1(double* pos)
{
	Rect1.x = 640;
	Rect1.y = 0;
	Rect1.width = 640;
	Rect1.height = 480;

	//drawViewportOutline(Rect1);

	// ofPushView() / ofPopView() are automatic
	camera1.begin(Rect1);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/* Cull back faces. */
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	//----Light-----
	setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	//----Camera Pos----
	//camera2.setPosition(ofVec3f(50,50,-350));
	//camera1.lookAt(ofVec3f(0.0f,0.0f,0.0f), ofVec3f(0.0f,1.0f,0.0f));
	//ofTranslate(0,0,0);
	ofDisableLighting();
	ofEnableLighting();


// 	ofPushMatrix();
// 	ofTranslate(m_positionApp[0], m_positionApp[1], m_positionApp[2]);
// 	//printf("pos : %lf\n", m_positionApp[2]);
// 	ofSetHexColor(0xFF0000);
// 	glutSolidSphere(10,200, 200);
// 	ofPopMatrix();
// 
// 
// 	ofPushMatrix();
// 	ofRotateY(90);
// 	ofTranslate(gBallPosition[0], gBallPosition[1], gBallPosition[2]);
// 	ofSetHexColor(0x00FFFF);
// 	glutSolidCube(gBallRadius*2);
// 	ofPopMatrix();
// 
// 
// 	ofPushMatrix();
// 	ofRotateY(90);
// 	if(!gtouch)		ofSetHexColor(0xFFFFFF);
// 	else			ofSetHexColor(0xFFFF00);
// 	
// 	glScalef(4,1,1);
// 	glutWireCube(gBallRadius*2);
// 	ofPopMatrix();
	
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	
	//--
	
	glDisable(GL_DEPTH_TEST);

	/* Cull back faces. */
	
	glDisable(GL_CULL_FACE);

	camera1.end();
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
bool __stdcall testApp::onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	
	if(nBufLen<=0)return false;
	unsigned char* packetData = (unsigned char*)pBuf;
	
	if(nChannel == SERVER_TO_CLIENT_1_HAPTIC_CHANNEL){
		hav::HapticPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->updateHapticInfo(packet);
		//printf("haptic : %d\n", hapticreceivecnt++);
		//printf("%f, %f, %f \n", packet.val.ballpos[0], packet.val.ballpos[1], packet.val.ballpos[2]);

		return true;
	}else if(nChannel == SERVER_TO_CLIENT_1_AUDIO_CHANNEL){
		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		char head = avio_r8(avioContext);
		if(head == '#'){
			int64_t time_stamp = avio_rb64(avioContext);
			AVPacket encoded_packet;
			av_init_packet(&encoded_packet);
			encoded_packet.data = (uint8_t*)packetData + (sizeof(char) + sizeof(int64_t));
			encoded_packet.size = nBufLen - (sizeof(char) + sizeof(int64_t));
			hav::packet_queue_put(&pTestApp->mAudioQueue, &encoded_packet, time_stamp);
			//printf("Audio : %d\n", audioreceivecnt++);
		}
		av_free(avioContext);
		
		return true;

	}else if(nChannel == SERVER_TO_CLIENT_1_VIDEO_CHANNEL){
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
			//printf("Video : %d\n", videoreceivecnt++);
		}
		av_free(avioContext);
		return true;
	}
	return false;
}
void testApp::updateHapticInfo(hav::HapticPacket packet)
{
	haptic_queue_put(hapticDevices.getHapticQueueReceiver(), &packet.val, 0);
/*	
	hpkt.val.pos[0] = packet.val.pos[0];
	hpkt.val.pos[1] = packet.val.pos[1];
	hpkt.val.pos[2] = packet.val.pos[2];
	gBallPosition[0] = packet.val.ballpos[0];
	gBallPosition[1] = packet.val.ballpos[1];
	gBallPosition[2] = packet.val.ballpos[2];
	gforce[0] = packet.val.Force[0];
	gforce[1] = packet.val.Force[1];
	gforce[2] = packet.val.Force[2];
	gtouch = packet.val.touch;
	//printf("%lf\n", gforce[2]);
	hav::haptic_queue_put(&mSyncHapticQueue, &packet.val, packet.val.timestamp);
	*/
}

void testApp::setup(){


	pTestApp = this;
	ofBackground(0, 0, 0);
	if(!mediaDecoder.initialize(MediaStreamClient, NULL, AV_CODEC_ID_MPEG1VIDEO)){
		printf("codec initialization failure \n");
	}
	hav::packet_queue_init(&mVideoQueue); hav::packet_queue_start(&mVideoQueue);
	hav::packet_queue_init(&mAudioQueue); hav::packet_queue_start(&mAudioQueue);
	hav::packet_queue_init(&mSyncAudioQueue); hav::packet_queue_start(&mSyncAudioQueue);
	hav::packet_queue_init(&mSyncVideoQueue); hav::packet_queue_start(&mSyncVideoQueue);
	hav::haptic_queue_init(&mHapticQueue); hav::haptic_queue_start(&mHapticQueue);
	hav::haptic_queue_init(&mSyncHapticQueue);hav::haptic_queue_start(&mSyncHapticQueue);


	swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLTP, 44100, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 44100,0, NULL);
	if (!swr_ctx || swr_init(swr_ctx) < 0) {
		printf(" swr context initialization failure\n");
	}

	std::map<int, std::string> mDevice;
	//if(mNetSender.Init("192.168.1.2",mDevice, "Content_Client_1", "", onDataReceived)){
	if(mNetSender.Init("114.70.63.186",mDevice, "Content_Client_1s", "", onDataReceived)){
		printf("Server connection is successful.");
		mNetSender.AddReceiveChannel(SERVER_TO_CLIENT_1_VIDEO_CHANNEL);
 		mNetSender.AddReceiveChannel(SERVER_TO_CLIENT_1_AUDIO_CHANNEL);
 		mNetSender.AddReceiveChannel(SERVER_TO_CLIENT_1_HAPTIC_CHANNEL);
		mNetSender.startThread();
	}

	//ofSetVerticalSync(true);
	//ofSetCircleResolution(80);
	

	// ==== By KHH ========================================= 
	vector<int> id;
	id.push_back(0);
	hapticDevices.initialize(new HapticWorldModel(id), id);
	// ----------------------------------------------------


	mGlImageTexture.allocate(WIDTH,HEIGHT,GL_RGB);

	//Falcon_Init();
	Audio_init();

	setGUI1();
	gui1->setDrawBack(false);

	ofEnableSmoothing();


	videoreceivecnt = 0;
	audioreceivecnt = 0;
	hapticreceivecnt = 0;
	sync_error_time =0;
	priorpacketCnt=0;

	SDL_CreateThread(Sync, (void*)this);

	Save_flag = false;
	buff_flag = 0;
	VideoTimeFile = fopen("VideoTime.txt","w");
	AudioTimeFile= fopen("AudioTime.txt","w");
	HAPTICTimeFile= fopen("HapticTime.txt","w");
	
	
	dt=0;

	t_i=0;
	t_j=0;
	T_i=0;
	T_i=0;

	// ==== By KHH ========================================= 
	setTimerEventScheduler();
	// ----------------------------------------------------
}


void testApp::Audio_init()
{
	av_log_set_level(AV_LOG_DEBUG);

	AudioDecodcodec = avcodec_find_decoder(AV_CODEC_ID_AAC);

	if (!AudioDecodcodec) {
		fprintf(stderr, "codec not found\n");

	}
	AudioDecodeContext = avcodec_alloc_context3(AudioDecodcodec);
	if (avcodec_open2(AudioDecodeContext, AudioDecodcodec, NULL) < 0) 
		fprintf(stderr, "could not open codec\n");

	int bufferSize		= 441;
	sampleRate			= 44100;
	lAudio.assign(bufferSize, 0.0);
	soundStream.setup(this, 1, 0, sampleRate, bufferSize, 1);

	audio_tgt.fmt = AV_SAMPLE_FMT_S16;
	audio_tgt.freq = 44100;
	audio_tgt.channel_layout = av_get_default_channel_layout(1);
	audio_tgt.channels =  1;

	audio_src = audio_tgt;
	errfile = fopen("aa.txt", "w");

	

}

void testApp::setGUI1()
{
	
	float red = 233; float blue = 52; float green = 27; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
	float length = 255-xInit; 
	float dim = 16; 

	vector<string> Audio_names; 
	Audio_names.push_back("Audio Bit rate 30000");
	Audio_names.push_back("Audio Bit rate 100000");
	Audio_names.push_back("Audio Bit rate 300000");
	
	vector<string> Video_names; 
	Video_names.push_back("Video Bit rate 30000");
	Video_names.push_back("Video Bit rate 100000");
	Video_names.push_back("Video Bit rate 300000");

	vector<string> Buffer_nemes;
	Buffer_nemes.push_back("Fix Buffer Size");
	Buffer_nemes.push_back("Adaptive Buffer size");
	Buffer_nemes.push_back("non-Buffer size");


	gui1 = new ofxUICanvas(0, 0, length+xInit, ofGetHeight()); 
	gui1->addWidgetDown(new ofxUILabel("Client1", OFX_UI_FONT_LARGE)); 
// 	gui1->addSpacer(length-xInit, 2);
// 	gui1->addRadio("RADIO VERTICAL", Audio_names, OFX_UI_ORIENTATION_VERTICAL, dim, dim); 
// 	gui1->addSpacer(length-xInit, 2);
// 	gui1->addRadio("Video Bit rate", Video_names, OFX_UI_ORIENTATION_VERTICAL, dim, dim); 
// 	gui1->addSpacer(length-xInit, 2);
	gui1->addRadio("Buffer Size", Buffer_nemes, OFX_UI_ORIENTATION_VERTICAL, dim, dim); 
	gui1->addSpacer(length-xInit, 2);
	gui1->addWidgetDown(new ofxUILabel("MOVING GRAPH", OFX_UI_FONT_MEDIUM)); 				    
	

	vector<float> buffer; 
	for(int i = 0; i < 256; i++)
	{
		buffer.push_back(0.0);
	}
	mg = (ofxUIMovingGraph *) gui1->addWidgetDown(
		new ofxUIMovingGraph(length-xInit, 120, buffer, 256, 1000, 100000, "MOVING GRAPH"));
	tex = (ofxUILabel*) gui1->addWidgetDown(
		new ofxUILabel ("AAA",OFX_UI_FONT_LARGE));



	ofAddListener(gui1->newGUIEvent,this,&testApp::guiEvent);



}

void testApp::guiEvent(ofxUIEventArgs &e){
	string name = e.widget->getName();
	int kind = e.widget->getKind(); 
	cout << "got event from: " << name << endl;
	CodecPacket cpacket;

	if(name == "Video Bit rate 30000")
	{
		cpacket.val.vodeo_bit_rate = 30000;
		mNetSender.SendData("Content_Servers", (char*)cpacket.udata, sizeof(CodecData), CLIENT_1_TO_SERVER_CODEC_CHANNEL);
		printf("Video Bit rate 30000\n");
	}
	if(name == "Video Bit rate 100000")
	{
		cpacket.val.vodeo_bit_rate = 100000;
		mNetSender.SendData("Content_Servers", (char*)cpacket.udata, sizeof(CodecData), CLIENT_1_TO_SERVER_CODEC_CHANNEL);
		printf("Video Bit rate 100000\n");
	}
	if(name == "Video Bit rate 300000")
	{
		cpacket.val.vodeo_bit_rate = 300000;
		mNetSender.SendData("Content_Servers", (char*)cpacket.udata, sizeof(CodecData), CLIENT_1_TO_SERVER_CODEC_CHANNEL);
		printf("Video Bit rate 300000\n");
	}

	if(name == "Fix Buffer Size")
	{
		
		buff_flag = 1;
		packet_queue_flush(&mVideoQueue);
		packet_queue_flush(&mAudioQueue);
		
	}

	if(name == "Adaptive Buffer size")
	{
		
		buff_flag = 2;
		
	}

	if(name == "non-Buffer size")
	{
		buff_flag = 0;
		packet_queue_flush(&mVideoQueue);
		packet_queue_flush(&mAudioQueue);
	}
}

//--------------------------------------------------------------
void testApp::update(){
	/*
	HapticPacket hapticpacket; 
	double Tdata[3];
	hapticpacket.val.pos[0] = -1*m_positionApp[2];
	hapticpacket.val.pos[1] = m_positionApp[1];
	hapticpacket.val.pos[2] = m_positionApp[0];
	hapticpacket.val.ballpos[0] = gBallPosition[0];
	hapticpacket.val.ballpos[1] = gBallPosition[1];
	hapticpacket.val.ballpos[2] = gBallPosition[2];
	mNetSender.SendData("Content_Servers", (char*)hapticpacket.udata, sizeof(hav::HapticData), CLIENT_1_TO_SERVER_HAPTIC_CHANNEL);
	*/
	
	dt = abs((t_i-t_j)-(T_i-T_j));
	mg->addPoint(dt);
	if(buff_flag == 1)
		tex->setLabel("Sync Error : " + ofToString(dt));
	else if(buff_flag ==0)
		tex->setLabel("Sync Error : - ");
}

//--------------------------------------------------------------
void testApp::draw(){

	// By HHKim ===============================
	CheckHapticButtonInput();
	aHapticDevice* hd = hapticDevices.findDevice(0);
	aState ballstate = hapticDevices.getHapticWorldModel()->getBallState();
	//drawHapticScene(hd->state.xt, ballstate.xt);
	// ---------------------------------------


	if(mSyncVideoQueue.nb_packets > 0){
		AVPacket vpacket;
		int64_t vtime_stamp;
		if(hav::packet_queue_get(&mSyncVideoQueue, &vpacket, 1, &vtime_stamp) >= 0) {
			//fprintf(VideoTimeFile, "%lld\t %lld%\n", vtime_stamp, av_gettime()+timeoffset);
			T_i = vtime_stamp;
			t_i = av_gettime()+timeoffset;
			AVFrame* decodedFrame = mediaDecoder.decodeVideoPacketToFrame(vpacket);
			if(decodedFrame != NULL){

				copyFrameDataToRgbBuffer(decodedFrame);
				mGlImageTexture.loadData((unsigned char*)(mRgbBuffer), WIDTH, HEIGHT, GL_RGB);
				

 			}
		}
	}
 	ofSetColor(0,0,0,0);
 	ofSetColor(255,255,255,255);
	mGlImageTexture.draw(0,0,ofGetWidth(),ofGetHeight());

	Canvas1(hpkt.val.pos);
	
	
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

	if(key == 's')
	{
		Save_flag = true;
	}
	if(key == 'q')
	{
		Save_flag = false;
		fclose(VideoTimeFile);
		fclose(AudioTimeFile);
		fclose(HAPTICTimeFile);

	}
	if(key == 'f')
	{
		ofToggleFullscreen(); 
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

void testApp::audioOut(float * output, int bufferSize, int nChannels){
	
	int audio_pkt_size = 0;
	AVFrame frame;
	int len1, data_size = 0;
	int resampled_data_size;
	int64_t time_stamp;


	if(mAudioQueue.nb_packets < 1){
		//return -1;
	}
	if(mSyncAudioQueue.nb_packets > 0)
	{

	
		if(hav::packet_queue_get(&mSyncAudioQueue, &pkt, 1, &time_stamp) > 0) {
			//fprintf(AudioTimeFile, "%lld\t %lld\n", time_stamp, av_gettime()+timeoffset);
			//printf("%lld, %lld\n", time_stamp, av_gettime()+timeoffset);
			T_j = time_stamp;
			t_j =av_gettime()+timeoffset;
		}
	}
	
	audio_pkt_size = pkt.size;
	{
	while(audio_pkt_size > 0)
	{
		
		int got_frame = 0;

		len1 = avcodec_decode_audio4(AudioDecodeContext , &frame, &got_frame, &pkt);

		if(len1 < 0) {
			/* if error, skip frame */
			audio_pkt_size = 0;
			break;
		}

		if (!got_frame){
			continue;
		}

		//---------------------------------------------------------------------------------------------------------

		audio_pkt_size -= len1;
		data_size = av_samples_get_buffer_size(	NULL, frame.channels,frame.nb_samples,(AVSampleFormat)frame.format,1);

		if(data_size <= 0) {
			// No data yet, get more frames 
			continue;
		}

		int wanted_nb_samples = frame.nb_samples;

		if (frame.format         != audio_src.fmt            ||
			frame.channel_layout != audio_src.channel_layout ||
			frame.sample_rate    != audio_src.freq           ||
			(wanted_nb_samples   != frame.nb_samples && !swr_ctx)) {
				swr_free(&swr_ctx);
				swr_ctx = swr_alloc_set_opts(
					NULL, 
					audio_tgt.channel_layout, 
					audio_tgt.fmt, 
					audio_tgt.freq, 
					frame.channel_layout, 
					(AVSampleFormat)frame.format, 
					//AV_SAMPLE_FMT_S16,
					frame.sample_rate,
					0, 
					NULL);
				swr_init(swr_ctx);

				audio_src.channel_layout = frame.channel_layout;
				audio_src.channels       = av_frame_get_channels(&frame);
				audio_src.freq			 = frame.sample_rate;
				audio_src.fmt			 = (AVSampleFormat)frame.format;
		}

		if (swr_ctx) 
		{
			uint8_t **out = &audio_buf1;
			int out_count = (int64_t)wanted_nb_samples * audio_tgt.freq / frame.sample_rate + 256;
			int out_size  = av_samples_get_buffer_size(NULL, audio_tgt.channels, frame.nb_samples, audio_tgt.fmt, 0);
			int len2;
			if (out_size < 0) {
				av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
				break;
			}

			av_fast_malloc(&audio_buf1, &audio_buf1_size, out_size);

			resampled_data_size = 441 * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);


			float* data = (float*)frame.data[0];
			float* idata = (float*) audio_buf1;
			int sindex = 1024-441;

			for(int i=1024-441; i<1024; i++)
			{
				float fdata = data[i];
				//fprintf(file_de, %"f\n", fdata);
				if(fdata > 1.0f) fdata = 1.0f;
				if(fdata < -1.0f) fdata = -1.0f;
				
				output[i-sindex] = (float)(fdata * 32767.0f);

			}
			//printf("resampled_data_size : %d\n",resampled_data_size);
			
		}
	}
	}
// 	else
// 		memset(output, 0, 1024);
}

void testApp::exit(){
	delete gui1;
}



/*
void testApp::testHDLError(const char* str)
{
	HDLError err = hdlGetError();
	if (err != HDL_NO_ERROR)
	{
		MessageBox(NULL, str, "HDAL ERROR", MB_OK);
		abort();
	}
}

void testApp::Falcon_Init()
{

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/ Cull back faces. 
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	HDLError err = HDL_NO_ERROR;

	m_deviceHandle = hdlInitNamedDevice("DEFAULT",0);
	testHDLError("hdlInitDevice");
	gLastTime = 0;

	gCursorDisplayList = 0;

	gCursorRadius = 0.05;
	gBallRadius = 100;
	gBallMass = 100;
	gBallViscosity = 80;
	gBallStiffness = 400;

	gBallPosition[0]=0;
	gBallPosition[1]=0;
	gBallPosition[2]=0;

	gforce[0] =0;
	gforce[1] =0;
	gforce[2] =0;

	gBallVelocity[0]=0;
	gBallVelocity[1]=0;
	gBallVelocity[2]=0;

	if (m_deviceHandle == HDL_INVALID_HANDLE)
	{
		MessageBox(NULL, "Could not open device", "Device Failure", MB_OK);
		exit();
	}

	// Now that the device is fully initialized, start the servo thread.
	// Failing to do this will result in a non-funtional haptics application.
	hdlStart();
	testHDLError("hdlStart");

	// Set up callback function
 	m_servoOp = hdlCreateServoOp(falcon_update, this, bNonBlocking);
 	//hdlCreateServoOp(touchScene2, this, bNonBlocking);
	if (m_servoOp == HDL_INVALID_HANDLE)
	{
		MessageBox(NULL, "Invalid servo op handle", "Device Failure", MB_OK);
	}
	testHDLError("hdlCreateServoOp");

	// Make the device current.  All subsequent calls will
	// be directed towards the current device.
	hdlMakeCurrent(m_deviceHandle);
	testHDLError("hdlMakeCurrent");

	hdlDeviceWorkspace(m_workspaceDims);
	testHDLError("hdlDeviceWorkspace");

	double gameWorkspace[] = {-100,-100,-150,100,100,600};
	bool useUniformScale = false;
	hdluGenerateHapticToAppWorkspaceTransform(m_workspaceDims,
		gameWorkspace,
		useUniformScale,
		m_transformMat);
	testHDLError("hdluGenerateHapticToAppWorkspaceTransform");

	// 	const bool bBlocking = false;
	// 	hdlCreateServoOp(touchScene,
	// 		this,
	// 		bBlocking);
	// 	testHDLError("hdlCreateServoOp"
}

void testApp::vecMultMatrix(double srcVec[3], double mat[16], double dstVec[3])
{
	dstVec[0] = mat[0] * srcVec[0] 
	+ mat[4] * srcVec[1]
	+ mat[8] * srcVec[2]
	+ mat[12];

	dstVec[1] = mat[1] * srcVec[0]
	+ mat[5] * srcVec[1]
	+ mat[9] * srcVec[2]
	+ mat[13];

	dstVec[2] = mat[2] * srcVec[0]
	+ mat[6] * srcVec[1]
	+ mat[10] * srcVec[2]
	+ mat[14];
}

// Interface function to get current position
void testApp::getPosition(double pos[3])
{
	pos[0] = m_positionApp[0];
	pos[1] = m_positionApp[1];
	pos[2] = m_positionApp[2];
	
}

double testApp::getSystemTime()
{
	static double s_wavelength = 0.0;
	static double s_wavelength_x_high = 0.0;
	static bool s_isFirstTime = true;

	if (s_isFirstTime)
	{
		LARGE_INTEGER l_frequency = { 0 };
		::QueryPerformanceFrequency(&l_frequency);
		s_wavelength = 1.0 / double(l_frequency.LowPart);
		s_wavelength_x_high = s_wavelength * double(MAXDWORD);
		s_isFirstTime = false;
	}

	LARGE_INTEGER l_ticks = { 0 };
	::QueryPerformanceCounter(&l_ticks);

	return s_wavelength_x_high * double(l_ticks.HighPart) +
		s_wavelength * double(l_ticks.LowPart);
}


// This is the callback function that is exercised at servo rate (1KHz)
HDLServoOpExitCode falcon_update(void* pUserData)
{
	double posDC[3];
	double posWC[3];
	double force[3];

	testApp* haptics = static_cast< testApp* >( pUserData );

	hdlMakeCurrent(haptics->m_deviceHandle);
	hdlToolPosition(haptics->m_positionServo);
	haptics->testHDLError("hdlToolPosition");
	haptics->vecMultMatrix(haptics->m_positionServo, haptics->m_transformMat, haptics->m_positionApp);

	hav::HapticData tdata;
	int64_t timestamp;
	hav::haptic_queue_get(&haptics->mSyncHapticQueue, &tdata, 1, &timestamp); 
	//fprintf(haptics->HAPTICTimeFile, "%lld\t %lld%\n", timestamp, av_gettime()+haptics->timeoffset);
	hdlSetToolForce(tdata.Force);

	haptics->testHDLError("hdlSetToolForce");

	return HDL_SERVOOP_CONTINUE;
}
*/
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
