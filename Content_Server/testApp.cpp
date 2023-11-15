#include "testApp.h"
testApp* pTestApp = NULL;
void touchScene1(void* pUserData);
void touchScene2(void* pUserData);
void touchScene1_1(void* pUserData);
void touchScene2_1(void* pUserData);

//--------------------------------------------------------------
typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

AudioParams audio_tgt;
AudioParams audio_src;
uint8_t *audio_buf1;
unsigned int audio_buf1_size;


// By HHKim
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
		mNetSender.SendData("Content_Client_1s", (char*)packet.udata, sizeof(hav::HapticData), SERVER_TO_CLIENT_1_HAPTIC_CHANNEL);
	}

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



static int dataSendingThread(void* data)
{
	testApp* app = (testApp*)data;

	for(;;){
		
// 		if (app->haptic_f1)
// 			app->getSocket()->SendData("Content_Client_1", (char*)app->Client1_packet.udata, sizeof(hav::HapticData), SERVER_TO_CLIENT_1_HAPTIC_CHANNEL);
// 		if(app->haptic_f2)
// 			app->getSocket()->SendData("Content_Client_2", (char*)app->Client2_packet.udata, sizeof(hav::HapticData), SERVER_TO_CLIENT_2_HAPTIC_CHANNEL);
		/*
		app->haptic_f1 = false;
		app->haptic_f2 = false;


		if(app->haptic.gethapticBuffer1()->nb_packets > 0){
			hav::HapticData packet;
			int64_t time_stamp;
			if(hav::haptic_queue_get(app->haptic.gethapticBuffer1(), &packet, 1, &time_stamp) < 0) {
				return -1;
			}
			uint8_t* buffer = NULL;
			{ 
				app->Client1_packet.val=packet;
				//if(app->Client1_packet.val.touch == true)
				app->getSocket()->SendData("Content_Client_1s", (char*)app->Client1_packet.udata, sizeof(hav::HapticData), SERVER_TO_CLIENT_1_HAPTIC_CHANNEL);

// 				app->Client1_packet.val.touch = false;
// 				app->Client2_packet.val.touch = false;
				av_free(buffer);
			}
		}
		*/

		if(app->ffmpeg.getAudioBuffer1()->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(app->ffmpeg.getAudioBuffer1(), &packet, 1, &time_stamp) < 0) {
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
				//if(app->Client1_packet.val.touch == true)
					app->getSocket()->SendData("Content_Client_1s", (char*)buffer, size, SERVER_TO_CLIENT_1_AUDIO_CHANNEL);

				//if(app->Client2_packet.val.touch == true)
					app->getSocket()->SendData("Content_Client_2s", (char*)buffer, size, SERVER_TO_CLIENT_2_AUDIO_CHANNEL);

				//app->Client1_packet.val.touch = false;
				//app->Client2_packet.val.touch = false;
				av_free(buffer);
			}
		}
		

		if(app->ffmpeg.getVideoBuffer1()->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(app->ffmpeg.getVideoBuffer1(), &packet, 1, &time_stamp) < 0) {
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
				app->getSocket()->SendData("Content_Client_1s", (char*)buffer, size, SERVER_TO_CLIENT_1_VIDEO_CHANNEL);
				
			
				av_free(buffer);
			}

		}
		if(app->ffmpeg.getVideoBuffer2()->nb_packets > 0){
			AVPacket packet;
			int64_t time_stamp;
			if(hav::packet_queue_get(app->ffmpeg.getVideoBuffer2(), &packet, 1, &time_stamp) < 0) {
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
				app->getSocket()->SendData("Content_Client_2s", (char*)buffer, size, SERVER_TO_CLIENT_2_VIDEO_CHANNEL);

				av_free(buffer);
			}

		}
		
		Sleep(1);
	}
}


void testApp::updateHapticInfo(hav::HapticPacket packet, int channel)
{
	if(channel == CLIENT_1_TO_SERVER_HAPTIC_CHANNEL)
	{
		
		haptic_queue_put(hapticDevices.getHapticQueueReceiver(), &packet.val, 0);
		//hapticDevices.getHapticQueue()
		//Client1_packet.val.pos[0] = packet.val.pos[0];
		//Client1_packet.val.pos[1] = packet.val.pos[1];
		//Client1_packet.val.pos[2] = packet.val.pos[2];
		//haptic_queue_put(&mReceiveQueue, &Client1_packet.val, 0);
		//haptic.touchScene1(&Client1_packet, &penetrationDist1);

	}
	else if(channel == CLIENT_2_TO_SERVER_HAPTIC_CHANNEL)
	{
		Client2_packet.val.pos[0] = packet.val.pos[0];
		Client2_packet.val.pos[1] = packet.val.pos[1];
		Client2_packet.val.pos[2] = packet.val.pos[2];
	}


}
void testApp::updateCodecInfo(CodecPacket packet, int channel)
{
	if(channel == CLIENT_1_TO_SERVER_CODEC_CHANNEL)
	{
		Client1_Codec_packet.val.audio_sample_rate = packet.val.audio_sample_rate;
		Client1_Codec_packet.val.audio_bit_rate = packet.val.audio_bit_rate;
		Client1_Codec_packet.val.video_sample_rate = packet.val.video_sample_rate;
		Client1_Codec_packet.val.video_bit_rate = packet.val.video_bit_rate;
		Client1_Codec_packet.val.etcData = packet.val.etcData;
		
		
		avcodec_close(ffmpeg.mVideoEncoderContext1);

		if(!ffmpeg.initializeVideoEncoder1(AV_CODEC_ID_MPEG1VIDEO, 640, 480, Client1_Codec_packet.val.video_bit_rate, 25))
			printf("initializeVideoEncoder ERROR\n");

	}
	else if(channel == CLIENT_2_TO_SERVER_CODEC_CHANNEL)
	{
		Client2_Codec_packet.val.audio_sample_rate = packet.val.audio_sample_rate;
		Client2_Codec_packet.val.audio_bit_rate = packet.val.audio_bit_rate;
		Client2_Codec_packet.val.video_sample_rate = packet.val.video_sample_rate;
		Client2_Codec_packet.val.video_bit_rate = packet.val.video_bit_rate;
		Client2_Codec_packet.val.etcData = packet.val.etcData;
		avcodec_close(ffmpeg.mVideoEncoderContext2);

		if(!ffmpeg.initializeVideoEncoder2(AV_CODEC_ID_MPEG1VIDEO, 640, 480, Client2_Codec_packet.val.video_bit_rate, 25))
			printf("initializeVideoEncoder ERROR\n");
	}
	

}
bool __stdcall testApp::onDataReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{

	if(nBufLen<=0)return false;
	unsigned char* packetData = (unsigned char*)pBuf;
	if(nChannel == CLIENT_1_TO_SERVER_HAPTIC_CHANNEL){
		hav::HapticPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->updateHapticInfo(packet,CLIENT_1_TO_SERVER_HAPTIC_CHANNEL);

		//printf("x : %lf\ny : %lf\nz: %lf\n",packet.val.pos[0], packet.val.pos[1], packet.val.pos[2]);
		return true;
	}
	else if(nChannel == CLIENT_2_TO_SERVER_HAPTIC_CHANNEL){
		hav::HapticPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->updateHapticInfo(packet,CLIENT_2_TO_SERVER_HAPTIC_CHANNEL);

		//printf("x : %lf\ny : %lf\nz: %lf\n",packet.val.pos[0], packet.val.pos[1], packet.val.pos[2]);
		return true;
	}
	else if(nChannel == CLIENT_1_TO_SERVER_CODEC_CHANNEL){
		CodecPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->updateCodecInfo(packet,CLIENT_1_TO_SERVER_CODEC_CHANNEL);

		//printf("x : %lf\ny : %lf\nz: %lf\n",packet.val.pos[0], packet.val.pos[1], packet.val.pos[2]);
		return true;
	}
	else if(nChannel == CLIENT_2_TO_SERVER_CODEC_CHANNEL){
		CodecPacket packet;
		memcpy(packet.udata, packetData, nBufLen);
		pTestApp->updateCodecInfo(packet,CLIENT_2_TO_SERVER_CODEC_CHANNEL);

		//printf("x : %lf\ny : %lf\nz: %lf\n",packet.val.pos[0], packet.val.pos[1], packet.val.pos[2]);
		return true;
	}

	return false;
}
void testApp::setup(){
	ofBackground(0, 0, 0);
	
	for (int i=0; i<3; i++)
	{
		Client1_packet.val.pos[i]=-600;
		Client2_packet.val.pos[i]=600;
	}

//	Client1_packet.val.touch = false;
//	Client2_packet.val.touch = false;

	haptic_f1 = false;
	haptic_f2 = false;


	pTestApp = this;

	std::map<int, std::string> mDevice;
	mDevice[0] = "Logitech Camera";
	if(mNetSender.Init(SERVER_IP_ADDRESS,mDevice, "Content_Servers", "", onDataReceived)){
		printf("Server connection is successful.");
		mNetSender.AddReceiveChannel(CLIENT_1_TO_SERVER_HAPTIC_CHANNEL);
		mNetSender.AddReceiveChannel(CLIENT_2_TO_SERVER_HAPTIC_CHANNEL);
		mNetSender.AddReceiveChannel(CLIENT_1_TO_SERVER_CODEC_CHANNEL);
		mNetSender.AddReceiveChannel(CLIENT_2_TO_SERVER_CODEC_CHANNEL);

		mNetSender.startThread();
	}
	swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLTP, 44100, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 44100,0, NULL);
	if (!swr_ctx || swr_init(swr_ctx) < 0) {
		printf(" swr context initialization failure\n");
	}

	hav::haptic_queue_init(&mReceiveQueue);
	hav::haptic_queue_start(&mReceiveQueue);
	SDL_CreateThread(dataSendingThread, (void*)this);
	
	ofSetVerticalSync(true);	
	ofSetCircleResolution(80);
	
	// ==== By KHH ========================================= 
	vector<int> id;
	id.push_back(0);
	hapticDevices.initialize(new HapticWorldModel(id), id, HAPTICMODE_VIRTUAL, 10); // set timer with 1ms period
	// ----------------------------------------------------
	
	ffmpeg.FFMPEG_Init(timeoffset);
	//Audio_init();
	//haptic.Haptic_Init(timeoffset);

// 	setGUI1();
// 	gui1->setDrawBack(false);
// 	ofEnableSmoothing();
	drawgl.bset();

	hapticSaveFile = fopen("hapticSaveFile1.txt","w");
	saveFlag = false;
	saveCnt =0;
// 	
	int bufferSize		= 441;
	sampleRate			= 44100;
 	soundStream.setup(this, 1, 0, sampleRate, bufferSize, 1);
	
	setTimerEventScheduler();

}





void testApp::setGUI1()
{
	float red = 233; float blue = 52; float green = 27; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
	float length = 255-xInit; 
	float dim = 16; 

	vector<string> names; 
	names.push_back("Sample rate 30000");
	names.push_back("Sample rate 100000");
	names.push_back("Sample rate 300000");
	
	
	gui1 = new ofxUICanvas(0, 0, length+xInit, ofGetHeight()); 
	gui1->addWidgetDown(new ofxUILabel("TestUI", OFX_UI_FONT_LARGE)); 
	gui1->addSpacer(length-xInit, 2);
	gui1->addRadio("RADIO VERTICAL", names, OFX_UI_ORIENTATION_VERTICAL, dim, dim); 
	
	ofAddListener(gui1->newGUIEvent,this,&testApp::guiEvent);
	
	

}

void testApp::guiEvent(ofxUIEventArgs &e){
	
}

//--------------------------------------------------------------
void testApp::update(){
	
	//haptic.touchScene2(&Client2_packet, &penetrationDist2);

// 	HapticPacket hdata;
// 	int64_t timestamp;
// 	if(haptic_queue_get(&mReceiveQueue,&hdata.val,1,&timestamp)>0)
// 	{
// 		//haptic.touchScene1(&hdata, &penetrationDist1);
// 	}
	
	haptic_f1 = true;
	haptic_f2 = true;
	
	if((saveFlag)&&(saveCnt<6000))
	{
		saveCnt++;
		int64_t ntime = av_gettime()+timeoffset;
		char buff[255]={0};
		sprintf(buff,"%f,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", (double)ntime
			, Client1_packet.val.pos[2]
			, Client1_packet.val.pos[1]
			, Client1_packet.val.pos[0] *-1
			, Client2_packet.val.pos[2] *-1
			, Client2_packet.val.pos[1]
			, Client2_packet.val.pos[0]
			, Client1_packet.val.Force[2]
			, Client2_packet.val.Force[2]);
		fprintf(hapticSaveFile, "%s", buff);
		printf("Cnt : %d\n", saveCnt);
	}
	if(saveCnt >6000)
		printf("Save Finish~!!!\n");

	
	
	
	
	
}

//--------------------------------------------------------------
void testApp::draw(){
	
	// ====== By HHKim  ===============
	CheckHapticButtonInput();
	aHapticDevice* hd = hapticDevices.findDevice(0);
	aState ballstate = hapticDevices.getHapticWorldModel()->getBallState();
	drawgl.drawHapticScene(hd->state.xt, ballstate.xt);
	// -----------------------------------
	static int cnt = 0;
	if(cnt++ % 100 == 0){
		hapticDevices.setThrowingCommand(1);
	}

	//drawgl.Canvas1(Client1_packet.val.pos, Client1_packet.val.pos, haptic.gBallPosition, haptic.gBallRadius);
	
	drawgl.bdraw();
	drawgl.bupdate();
	
	
	ffmpeg.VideoCapture();
//  	static int64_t old_utime = av_gettime();
//  	int64_t utime = av_gettime();
//  
//  	static double old_time = 0;
//  	double time = 0;
//  	printf("callback time  = %0.3f us \n",(double)(utime-old_utime));
//  
//  	old_time = time;
//  	old_utime = utime;
}
//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if( key == 'r')
	{
//		haptic.gBallPosition[0]=0;
//		haptic.gBallVelocity[0]=0;
	}
	
	if(key == 's')
	{
		saveFlag = true;
	}
	if(key == 'q')
	{
		saveFlag = false;
		fclose(hapticSaveFile);
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

	AVPacket pkt;
// 	av_init_packet(&pkt);
// 	pkt.size=0;
// 	pkt.data=NULL;
//  	int64_t timestamp;
// 	hav::packet_queue_get(&ffmpeg.mAudioTB,&pkt, 1, &timestamp);
// 	hav::packet_queue_put(&ffmpeg.mAudioBuffer1,&pkt,av_gettime());
// 	hav::packet_queue_put(&ffmpeg.mAudioTB,&pkt,timestamp);

//  	hav::packet_queue_put(&ffmpeg.mAudioBuffer1, &ffmpeg.AFpacket[0], av_gettime());
//  	hav::packet_queue_put(&ffmpeg.mAudioBuffer1, &ffmpeg.AFpacket[1], av_gettime());
	//ffmpeg.SoundCapture2();
	//AVFrame* tempAudioFrame = ffmpeg.SoundCapture(Client1_packet.val.touch,Client2_packet.val.touch,penetrationDist1, penetrationDist2);
//	AVFrame* tempAudioFrame = ffmpeg.SoundCapture(Client1_packet.val.touch,0,penetrationDist1, penetrationDist2);
	//float* data = (float*)tempAudioFrame->data[0];
	
// 	for(int i=0; i<441; i++)
// 	{
// 		float fdata = data[i];
// 		output[i] =(float) tempAudioFrame->data[i];
// 
// 

// 	int audio_pkt_size = 0;
// 	AVFrame frame;
// 	int len1, data_size = 0;
// 	int resampled_data_size;
// 	int64_t time_stamp;
// 
// 
// 	if(ffmpeg.mAudioTB.nb_packets < 1){
// 		//return -1;
// 	}
// 	if(hav::packet_queue_get(&ffmpeg.mAudioTB, &pkt, 1, &time_stamp) > 0) {
// 		//return -1;
// 	}
// 
// 	audio_pkt_size = pkt.size;
// 	{
// 		while(audio_pkt_size > 0)
// 		{
// 
// 			int got_frame = 0;
// 
// 			len1 = avcodec_decode_audio4(AudioDecodeContext , &frame, &got_frame, &pkt);
// 
// 			if(len1 < 0) {
// 				/* if error, skip frame */
// 				audio_pkt_size = 0;
// 				break;
// 			}
// 
// 			if (!got_frame){
// 				continue;
// 			}
// 
// 			//---------------------------------------------------------------------------------------------------------
// 
// 			audio_pkt_size -= len1;
// 			data_size = av_samples_get_buffer_size(	NULL, frame.channels,frame.nb_samples,(AVSampleFormat)frame.format,1);
// 
// 			if(data_size <= 0) {
// 				// No data yet, get more frames 
// 				continue;
// 			}
// 
// 			int wanted_nb_samples = frame.nb_samples;
// 
// 			if (frame.format         != audio_src.fmt            ||
// 				frame.channel_layout != audio_src.channel_layout ||
// 				frame.sample_rate    != audio_src.freq           ||
// 				(wanted_nb_samples   != frame.nb_samples && !swr_ctx)) {
// 					swr_free(&swr_ctx);
// 					swr_ctx = swr_alloc_set_opts(
// 						NULL, 
// 						audio_tgt.channel_layout, 
// 						audio_tgt.fmt, 
// 						audio_tgt.freq, 
// 						frame.channel_layout, 
// 						(AVSampleFormat)frame.format, 
// 						//AV_SAMPLE_FMT_S16,
// 						frame.sample_rate,
// 						0, 
// 						NULL);
// 					swr_init(swr_ctx);
// 
// 					audio_src.channel_layout = frame.channel_layout;
// 					audio_src.channels       = av_frame_get_channels(&frame);
// 					audio_src.freq			 = frame.sample_rate;
// 					audio_src.fmt			 = (AVSampleFormat)frame.format;
// 			}
// 
// 			if (swr_ctx) 
// 			{
// 				uint8_t **out = &audio_buf1;
// 				int out_count = (int64_t)wanted_nb_samples * audio_tgt.freq / frame.sample_rate + 256;
// 				int out_size  = av_samples_get_buffer_size(NULL, audio_tgt.channels, frame.nb_samples, audio_tgt.fmt, 0);
// 				int len2;
// 				if (out_size < 0) {
// 					av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
// 					break;
// 				}
// 
// 				av_fast_malloc(&audio_buf1, &audio_buf1_size, out_size);
// 
// 				resampled_data_size = 441 * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);
// 
// 
// 				float* data = (float*)frame.data[0];
// 				float* idata = (float*) audio_buf1;
// 				int sindex = 1024-441;
// 
// 				for(int i=1024-441; i<1024; i++)
// 				{
// 					float fdata = data[i];
// 					//fprintf(file_de, "%f\n", fdata);
// 					if(fdata > 1.0f) fdata = 1.0f;
// 					if(fdata < -1.0f) fdata = -1.0f;
// 
// 					output[i-sindex] = (float)(fdata * 32767.0f);
// 
// 				}
// 				//printf("resampled_data_size : %d\n",resampled_data_size);
// 
// 			}
// 		}
 }
void testApp::Audio_init()
{
	//av_log_set_level(AV_LOG_DEBUG);

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
	audio_tgt.freq = 44100/2;
	audio_tgt.channel_layout = av_get_default_channel_layout(1);
	audio_tgt.channels =  1;

	audio_src = audio_tgt;
}