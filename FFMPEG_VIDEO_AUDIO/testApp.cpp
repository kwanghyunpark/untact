#include "testApp.h"
testApp* pTestApp = NULL;
// By HHKim
void testApp::setTimerEventScheduler()
{
	int desired_ms = 4; // for haptic

	// 아래함수를 사용하려면 project/ settings | c/c++ | preprocessor definitions 에 "_WIN32_WINNT=0x0500"을 넣어주어야 함
	//BOOL success = CreateTimerQueueTimer(&handleHapticDataSerder, NULL, (WAITORTIMERCALLBACK)_intraQueueTimerEventListenerForHapticDataSender,this, 0, desired_ms, WT_EXECUTEINTIMERTHREAD);
}

void testApp::killTimerEventScheduler()
{
	//BOOL res = DeleteTimerQueueTimer(NULL, handleHapticDataSerder, NULL);

}

void testApp::setup(){
	ofBackground(0, 0, 0);

	pTestApp = this;

	ofSetVerticalSync(true);	
	ofSetCircleResolution(80);


	int inp_flag;
#ifdef INPUT_CAPTURE_SIN
	inp_flag = INPUT_CAPTURE_SIN;
#endif
#ifdef INPUT_MEDIAFILE
	inp_flag = INPUT_MEDIAFILE;
#endif
	ffmpeg.FFMPEG_Init(timeoffset, inp_flag);

	drawgl.bset();

	
	saveFlag = false;
	saveCnt =0;
	// 	
	int bufferSize		= 512;
	sampleRate			= 48000;
	phase 				= 0;
	phaseAdder 			= 0.0f;	
	phaseAdderTarget 	= 0.0f;
	volume				= 0.1f;

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
	soundStream.setup(this, 2, 0, sampleRate, bufferSize, 1);
	mGlImageTexture.allocate(VIDEO_WIDTH,VIDEO_HEIGHT,GL_RGB);

	//setTimerEventScheduler();

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


}

//--------------------------------------------------------------
void testApp::copyFrameDataToRgbBuffer(AVFrame* decodedFrame)
{
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, decodedFrame->width, decodedFrame->height);
	//uint8_t *buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameRGB, mRgbBuffer, AV_PIX_FMT_RGB24, decodedFrame->width, decodedFrame->height);
	SwsContext* img_convert_ctx = sws_getContext(decodedFrame->width, decodedFrame->height,AV_PIX_FMT_YUV420P, decodedFrame->width, decodedFrame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, decodedFrame->data, decodedFrame->linesize, 0, decodedFrame->height, pFrameRGB->data, pFrameRGB->linesize); 
	av_free(img_convert_ctx);
	av_free(pFrameRGB);
}
void testApp::draw(){

	
	double pos=10;
	double state=0;
#ifdef INPUT_CAPTURE_SIN
	drawgl.drawHapticScene(&pos, &state);
	drawgl.bdraw();
	drawgl.bupdate();
	ffmpeg.VideoCapture();
#endif

	AVPacket vpacket;
	int64_t vtime_stamp;

	if(ffmpeg.getVideoBuffer()->nb_packets > 0) 
	{
		hav::packet_queue_get(ffmpeg.getVideoBuffer(), &vpacket, 1, &vtime_stamp);
		
		AVFrame* decodedFrame = ffmpeg.VideoDecode(vpacket);
		if(decodedFrame != NULL)
		{

			copyFrameDataToRgbBuffer(decodedFrame);
			
#ifdef	INPUT_CAPTURE_SIN
			mGlImageTexture.draw(640,0,640,480);
			mGlImageTexture.loadData((unsigned char*)(mRgbBuffer), 640, 480, GL_RGB);
#endif

#ifdef	INPUT_MEDIAFILE
			mGlImageTexture.draw(0,0,1280,720);
			mGlImageTexture.loadData((unsigned char*)(mRgbBuffer), 1280, 720, GL_RGB);
#endif
		}
	}


}
//--------------------------------------------------------------
void testApp::keyPressed(int key){
	
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
void testApp::audioOut(float *output, int bufferSize, int nChannels){

	AVPacket pkt;
	int64_t time_stamp;
	pkt.size=0;
#ifdef INPUT_CAPTURE_SIN
	ffmpeg.SoundCapture();
#endif
	if(ffmpeg.mAudioEncodeBuffer.nb_packets>0)
	{
		hav::packet_queue_get(&ffmpeg.mAudioEncodeBuffer, &pkt, 1, &time_stamp);
		ffmpeg.AudioDecode(pkt, output);
		
		for (int i = 0; i < bufferSize; i++){
			lAudio[i] = output[i*nChannels    ]; 
			rAudio[i] = output[i*nChannels + 1];
		}		
	}
	//FILE* aout2 = fopen("out2.txt", "w");
		
//	{
// 		float phase=0;
// 			float phaseAdder=0;
// 			int sampleRate=44100;
// 			int bufferSize= 1023;
// 			int nChannels=1;
// 			float volume=0.1;
// 	
// 			float targetFrequency = 2000.0f *10;//* ballstate.xt[2];
// 			float phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
// 			phaseAdder = 0.05f * phaseAdder + 0.05f * phaseAdderTarget;
// 	 		for (int i = 0; i < bufferSize; i++){
// 	 			phase += phaseAdder;
// 				//printf("INP:%f\n",phaseAdder );
// 	 			float sample = sin(phase);
// 				output[i*nChannels] = sample * volume ;
// 				//fprintf(aout2,"%f\n",output[i]);
// 	 			//output[i*nChannels + 1] = sample * volume ;
// 	 		}
// 	
// 		}
	//fclose(aout2);



}
