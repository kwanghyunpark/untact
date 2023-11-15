#include "FFMPEG.h"


int VideoEncode1(void*);
int VideoEncode2(void*);


FFMPEG::FFMPEG(void)
{

}


FFMPEG::~FFMPEG(void)
{
}
bool FFMPEG::initializeVideoEncoder1(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size)
{

	hav::packet_queue_flush(&mVideoBuffer1);

	mVideoEncoder1 = avcodec_find_encoder(codecId);
	mVideoEncoderContext1 = avcodec_alloc_context3(mVideoEncoder1);
	mVideoEncoderContext1->bit_rate = bitRate;
	mVideoEncoderContext1->width = width;
	mVideoEncoderContext1->height = height;
	mVideoEncoderContext1->time_base.num = 1;
	mVideoEncoderContext1->time_base.den = 25;
	mVideoEncoderContext1->gop_size = gop_size;
	mVideoEncoderContext1->max_b_frames = 1;
	mVideoEncoderContext1->mb_decision  = 2;
	mVideoEncoderContext1->pix_fmt = AV_PIX_FMT_YUV420P;
	if(avcodec_open2(mVideoEncoderContext1, mVideoEncoder1, NULL) < 0){
		printf("encoder setting failure \n");
		return false;
	}
	


	return true;
}

bool FFMPEG::initializeVideoEncoder2(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size)
{

	mVideoEncoder2 = avcodec_find_encoder(codecId);
	mVideoEncoderContext2 = avcodec_alloc_context3(mVideoEncoder2);
	mVideoEncoderContext2->bit_rate = bitRate;
	mVideoEncoderContext2->width = width;
	mVideoEncoderContext2->height = height;
	mVideoEncoderContext2->time_base.num = 1;
	mVideoEncoderContext2->time_base.den = 25;
	mVideoEncoderContext2->gop_size = gop_size;
	mVideoEncoderContext2->max_b_frames = 1;
	mVideoEncoderContext2->mb_decision  = 2;
	mVideoEncoderContext2->pix_fmt = AV_PIX_FMT_YUV420P;
	if(avcodec_open2(mVideoEncoderContext2, mVideoEncoder2, NULL) < 0){
		printf("encoder setting failure \n");
		return false;
	}
	


	return true;
}

void FFMPEG::initializeVideoCapture()
{
	pPixeldata = new GLbyte[640*480*3];

	CapturedFrame = avcodec_alloc_frame();
	pictYUV= avcodec_alloc_frame();
	CapturedFrame->format = AV_PIX_FMT_BGR24;
	CapturedFrame->width = 640;
	CapturedFrame->height = 480;
	av_image_alloc(CapturedFrame->data, CapturedFrame->linesize, 640,480,AV_PIX_FMT_BGR24, 32);

	//glGetIntegerv(GL_VIEWPORT, iViewPort);

	iViewPort[2] = 640;
	iViewPort[3] = 480;

	unsigned long lImagesize = iViewPort[2]*3*iViewPort[3];
	//printf("%d, %d, %d\n", lImagesize, iViewPort[2], iViewPort[3]);

	outbuf_size = 100000;
	outbuf = (uint8_t *)malloc(outbuf_size);
	size =iViewPort[2]*iViewPort[3];

	CapturedFrame->data[0] = (uint8_t*)pPixeldata+iViewPort[2]*3*(iViewPort[3]-1);
	CapturedFrame->data[1] = CapturedFrame->data[0] + size;
	CapturedFrame->data[2] = CapturedFrame->data[1] + size;
	CapturedFrame->linesize[0] =-3*iViewPort[2];
	CapturedFrame->linesize[1] =-3*iViewPort[2];
	CapturedFrame->linesize[2] =-3*iViewPort[2];

	size = 640*480;
	pictYUV_buf = (uint8_t*)malloc((size * 3) / 2); /* size for YUV 420 */
	pictYUV->data[0] = pictYUV_buf;
	pictYUV->data[1] = pictYUV->data[0] + size;
	pictYUV->data[2] = pictYUV->data[1] + size / 4;
	pictYUV->linesize[0] = 640;
	pictYUV->linesize[1] = 640 / 2;
	pictYUV->linesize[2] = 640 / 2; 
	img_convert_ctx = sws_getContext(iViewPort[2], iViewPort[3],
		AV_PIX_FMT_BGR24, 640, 480, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 	NULL, NULL, NULL);

	frameno=0;
	//각 클라이언트에 다른 인코딩 옵션 적용
	SDL_CreateThread(VideoEncode1, this);
	//SDL_CreateThread(VideoEncode2, this);
}



bool FFMPEG::initializeAudioEncoder1(enum AVCodecID codecId, int width, int height, int bitRate, int sample_rate)
{
	mAudioEncoder = avcodec_find_encoder(codecId);
	mAudioEncoderContext = avcodec_alloc_context3(mAudioEncoder);

	// ----------------------------------
	// setting audio encoder context hear
	// ----------------------------------

	mAudioEncoderContext->sample_fmt = AV_SAMPLE_FMT_S16;
	mAudioEncoderContext->bit_rate = bitRate;
	mAudioEncoderContext->sample_rate = sample_rate;
	mAudioEncoderContext->channel_layout = av_get_default_channel_layout(1);
	mAudioEncoderContext->channels = av_get_channel_layout_nb_channels(mAudioEncoderContext->channel_layout);
	mAudioEncoderContext->profile = FF_PROFILE_AAC_LOW;

	// ----------------------------------

	if(avcodec_open2(mAudioEncoderContext, mAudioEncoder, NULL) < 0){
		printf("encoder setting failure \n");
		return false;
	}

	audioFrame = avcodec_alloc_frame();
	audioFrame->nb_samples     = mAudioEncoderContext->frame_size;
	audioFrame->format         = mAudioEncoderContext->sample_fmt;
	audioFrame->channel_layout = mAudioEncoderContext->channel_layout;
	audio_buffer_size = av_samples_get_buffer_size(NULL, mAudioEncoderContext->channels,
		mAudioEncoderContext->frame_size,
		mAudioEncoderContext->sample_fmt, 0);
	audio_sample = (uint16_t*)av_malloc(audio_buffer_size);
	int ret = avcodec_fill_audio_frame(audioFrame, mAudioEncoderContext->channels,
		mAudioEncoderContext->sample_fmt,
		(const uint8_t*)audio_sample, audio_buffer_size, 0);


	return true;
}

void FFMPEG::FFMPEG_Init(int64_t t)
{
	timeoffset=t;
	av_register_all(); 
	avcodec_register_all();

	hav::packet_queue_init(&mVideoBuffer1);
	hav::packet_queue_init(&mAudioBuffer1);
	hav::packet_queue_start(&mVideoBuffer1);
	hav::packet_queue_start(&mAudioBuffer1);
	
	hav::packet_queue_init(&mVideoBuffer2);
	hav::packet_queue_init(&mAudioBuffer2);
	hav::packet_queue_start(&mVideoBuffer2);
	hav::packet_queue_start(&mAudioBuffer2);

	
	if(!initializeVideoEncoder1(AV_CODEC_ID_MPEG1VIDEO, 640, 480, 100000, 25))
		printf("initializeVideoEncoder ERROR\n");
 	if(!initializeAudioEncoder1(AV_CODEC_ID_AAC,0,0,32000,44100/2))
 		printf("initializeAudioEncoder EEOR\n");
	//if(!initializeVideoEncoder2(AV_CODEC_ID_MPEG1VIDEO, 640, 480, 30000, 25))
		//printf("initializeVideoEncoder ERROR\n");
//  	if(!initializeAudioEncoder2(AV_CODEC_ID_AAC,0,0,32000,44100))
//  		printf("initializeAudioEncoder EEOR\n");
	initializeVideoCapture();
	//initializeAudioFileDecoderEncoder();
	//InitializeCriticalSection(&cs);

}

AVFrame* FFMPEG::SoundCapture(bool touch1, bool touch2, double dis1, double dis2)
{
	float t, tincr;
	AVPacket pkt;

	av_init_packet(&pkt);
	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	uint8_t fdata;
	uint8_t sdata;
	int16_t ttdata;
	for (int j = 0; j < audioFrame->linesize[0]; j+=2) 
	{
		if(touch1||touch2)
		{
			ttdata = (int16_t)(sin((3.14*j*3)/0.08)*65536) ;
			//printf("%d\n", ttdata);
		}		
		else
			ttdata=0;
		
		fdata = (uint8_t) ((ttdata & 0xff00) >> 8);
		sdata = (uint8_t) ((ttdata & 0x00ff) >> 0);
		audioFrame->data[0][j] = sdata;
		audioFrame->data[0][j+1] = fdata;
	}

	int got_output;
	int64_t temptime = av_gettime()+timeoffset;
	int ret = avcodec_encode_audio2(mAudioEncoderContext, &pkt, audioFrame, &got_output);
	if (ret < 0) 	fprintf(stderr, "Error encoding audio frame\n");
	else
	{
		hav::packet_queue_put(&mAudioBuffer1, &pkt, temptime);
		av_free_packet(&pkt);
	}
	return audioFrame;
}
//OpenGL 화면 캡쳐
void FFMPEG::VideoCapture()
{
	//EnterCriticalSection(&cs);
	glReadPixels(0,0,640,480,GL_BGR_EXT,GL_UNSIGNED_BYTE, pPixeldata);
	Capture_time = av_gettime()+timeoffset;
	//printf("%lld \n", timeoffset);
	if(!sws_scale(img_convert_ctx, CapturedFrame->data, 
		CapturedFrame->linesize, 0, iViewPort[3],pictYUV->data, pictYUV->linesize))
		printf("Error\n");
	//LeaveCriticalSection(&cs);

	//인코딩 루틴의 주기가 VideoCapture 루틴에 비해 빠르다.
	captureflag1 = true;
	captureflag2 = true;
}
int VideoEncode1(void* data1)
{
	
		FFMPEG* app = (FFMPEG*)data1;
		while(1){
			if(app->captureflag1 == true)
			{
				int packet_size1;
				av_init_packet(&app->encoded_packet1);
				app->encoded_packet1.data = NULL;
				app->encoded_packet1.size = 0;
				int got_packet1;
				//EnterCriticalSection(&app->cs);
				int res1 = avcodec_encode_video2(app->mVideoEncoderContext1, &app->encoded_packet1, app->pictYUV, &got_packet1);
				//LeaveCriticalSection(&app->cs);
				//	int16_t time_stamp1 = av_gettime()+timeoffset;

				if(res1 == 0 && got_packet1 > 0){
					hav::packet_queue_put(&app->mVideoBuffer1, &app->encoded_packet1, app->Capture_time);
					packet_size1 = app->encoded_packet1.size;
					//printf("%f \n", (double)app->Capture_time);
				}else{
					packet_size1 = -1;
					av_free_packet(&app->encoded_packet1);
				}
				app->captureflag1 = false;
			}
	}
	return 0;
}

int VideoEncode2(void* data2)
{
	FFMPEG* app = (FFMPEG*)data2;
	while(1)
	{
		if(app->captureflag2 == true)
		{
			int packet_size2;
			av_init_packet(&app->encoded_packet2);
			app->encoded_packet2.data = NULL;
			app->encoded_packet2.size = 0;
			int got_packet2;
			//EnterCriticalSection(&app->cs);
			int res2 = avcodec_encode_video2(app->mVideoEncoderContext2, &app->encoded_packet2, app->pictYUV, &got_packet2);
			//LeaveCriticalSection(&app->cs);
			int64_t time_stamp2 = 0;

			if(res2 == 0 && got_packet2 > 0){
				hav::packet_queue_put(&app->mVideoBuffer2, &app->encoded_packet2, time_stamp2);
				packet_size2 = app->encoded_packet2.size;
				//printf("%d \n", packet_size);
			}else{
				packet_size2 = -1;
				av_free_packet(&app->encoded_packet2);
			}
			app->captureflag2 = false;
		}
	}
	return 0;
}


bool FFMPEG::initializeAudioFileDecoderEncoder()
{
	/*

	hav::packet_queue_init(&mAudioTB);
	hav::packet_queue_start(&mAudioTB);

	pFormatCtx = avformat_alloc_context();
	const char FileName[] = "../bin/data/es040.wav";
	const char inputFormatName[] ="dshow";
	options = NULL;
	av_dict_set(&options, "samle_rate", "44100", 0);
	av_dict_set(&options, "channels","2",0);
	av_dict_set(&options, "audio_buffer_size", "10", 0);	
	
	iformat = av_find_input_format(FileName);
	// retrieve stream information 

	if (avformat_open_input(&pFormatCtx, FileName, iformat, &options) < 0) {
		fprintf(stderr, "Could not open source file %s\n", FileName);
		//exit(1);
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		//exit(1);
	}
	av_dump_format(pFormatCtx, 0, FileName, false);
	
	videoStream=-1;
	audioStream=-1;
	for(int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
			videoStream < 0) {
				videoStream=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
			audioStream < 0) {
				audioStream=i;
		}
	}
	aCodecCtx = NULL;
	aCodecCtx = pFormatCtx->streams[audioStream]->codec;
	aCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_S16;
	aCodecCtx->channels=2;
	aCodecCtx->channel_layout=av_get_default_channel_layout(aCodecCtx->channels);
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	AVDictionary  *audioOptionsDict  = NULL;
	avcodec_open2(aCodecCtx, aCodec, &audioOptionsDict);
	*/


	FILE * infile = fopen("../bin/data/1.wav", "rb");
	int BUFFSIZE = 256;
	int cnt=0;
	short int buff16[256];
	header_p meta = (header_p)malloc(sizeof(header));;
	int nb;
	if(infile)
	{
		fread(meta, 1, sizeof(header), infile);
		cout << " Size of Header file is "<<sizeof(*meta)<<" bytes" << endl;  
		cout << " Sampling rate of the input wave file is "<< meta->sample_rate <<" Hz" << endl;  
		cout << " Number of samples in wave file are " << meta->subchunk2_size << " samples" << endl;  
		cout << " The number of channels of the file is "<< meta->num_channels << " channels" << endl; 

	
	while (!feof(infile))  
	{  
		nb = fread(buff16,1,BUFFSIZE,infile);        // Reading data in chunks of BUFSIZE  
		//cout << nb <<endl;  
		cnt++;                    // Incrementing Number of frames  

	}  

	cout << " Number of frames in the input wave file are " <<cnt << endl;  

}  

	
	SoundCapture2();


	return 0;

}

AVFrame* FFMPEG::SoundCapture2()
{
	



	/*
	AVFrame* aFrame = avcodec_alloc_frame(); 
	//int readflag=av_read_frame(pFormatCtx, &packet);
	int icnt=0;
	while(av_read_frame(pFormatCtx, &AFpacket[icnt])>=0){
		if(AFpacket[icnt].stream_index == audioStream) {
			int frameFinished = -1;
			
			int aa = avcodec_decode_audio4(aCodecCtx, aFrame, &frameFinished, &AFpacket[icnt]);
			if(frameFinished){
				int got_output = -1;
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = NULL; // packet data will be allocated by the encoder
				pkt.size = 0;
				int ret = avcodec_encode_audio2(mAudioEncoderContext, &pkt, aFrame, &got_output);
				if (ret < 0) 	fprintf(stderr, "Error encoding audio frame\n");
				else
					hav::packet_queue_put(&mAudioTB, &pkt, av_gettime()+timeoffset);
				//av_free_packet(&pkt);
				//hav::packet_queue_put(&mAudioBuffer1, &packet[icnt++], av_gettime()+timeoffset);
			}
		}
	}

	printf("icnt : %d\n", icnt);
	return aFrame;
	*/
	return 0;
}