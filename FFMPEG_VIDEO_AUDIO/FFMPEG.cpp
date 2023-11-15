#include "FFMPEG.h"


int VideoEncodeThread(void*);
int Media_Decode_thread(void*);
int stream_component_open(FFMPEG *is, int stream_index);


FFMPEG::FFMPEG(void)
{

}


FFMPEG::~FFMPEG(void)
{
}
bool FFMPEG::initializeVideoEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size)
{

	hav::packet_queue_flush(&mVideoEncodeBuffer);

	mVideoEncoder = avcodec_find_encoder(codecId);
	mVideoEncoderContext = avcodec_alloc_context3(mVideoEncoder);
	mVideoEncoderContext->bit_rate = bitRate;
	mVideoEncoderContext->width = width;
	mVideoEncoderContext->height = height;
	mVideoEncoderContext->time_base.num = 1;
	mVideoEncoderContext->time_base.den = 25;
	mVideoEncoderContext->gop_size = gop_size;
	mVideoEncoderContext->max_b_frames = 1;
	mVideoEncoderContext->mb_decision  = 2;
	mVideoEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	if(avcodec_open2(mVideoEncoderContext, mVideoEncoder, NULL) < 0){
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
	SDL_CreateThread(VideoEncodeThread, this);
	//SDL_CreateThread(VideoDecode, this);
	//SDL_CreateThread(VideoEncode2, this);
}



bool FFMPEG::initializeAudioEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int sample_rate)
{
	mAudioEncoder = avcodec_find_encoder(codecId);
	mAudioEncoderContext = avcodec_alloc_context3(mAudioEncoder);

	// ----------------------------------
	// setting audio encoder context hear
	// ----------------------------------

	mAudioEncoderContext->sample_fmt = AV_SAMPLE_FMT_S16;
	mAudioEncoderContext->bit_rate = bitRate;
	mAudioEncoderContext->sample_rate = sample_rate;
	mAudioEncoderContext->channel_layout = av_get_default_channel_layout(2);
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

bool FFMPEG::initializeAudioDecoder(enum AVCodecID codecId)
{
	mAudioDecoder = avcodec_find_decoder(codecId);
	mAudioDecoderContext = avcodec_alloc_context3(mAudioDecoder);

	if(avcodec_open2(mAudioDecoderContext, mAudioDecoder, NULL) < 0){
		fprintf(stderr, "fail to open decoder \n");
		return false; // Could not open codec
	}
	audio_tgt.fmt = AV_SAMPLE_FMT_FLT;
	audio_tgt.freq = 48000;
	audio_tgt.channel_layout = av_get_default_channel_layout(2);
	audio_tgt.channels =  2;

	audio_src = audio_tgt;
	audio_buf1 = NULL;
	swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 
		audio_tgt.freq, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, 48000,0, NULL);
	if (!swr_ctx || swr_init(swr_ctx) < 0) {
		printf(" swr context initialization failure\n");
	}

	apts=0;
	return true;
}
bool FFMPEG::initializeVideoDecoder(enum AVCodecID codecId)
{
	mVideoDecoder = avcodec_find_decoder(codecId);
	mVideoDecoderContext = avcodec_alloc_context3(mVideoDecoder);

	if(avcodec_open2(mVideoDecoderContext, mVideoDecoder, NULL) < 0){
		fprintf(stderr, "fail to open decoder \n");
		return false; // Could not open codec
	}

	return true;
}


void FFMPEG::FFMPEG_Init(int64_t t, int inp_flag)
{
	timeoffset=t;
	av_register_all(); 
	avcodec_register_all();
	
	hav::packet_queue_init(&mVideoEncodeBuffer);
	hav::packet_queue_init(&mAudioEncodeBuffer);
	hav::packet_queue_start(&mVideoEncodeBuffer);
	hav::packet_queue_start(&mAudioEncodeBuffer);
	
	hav::packet_queue_init(&mVideoBuffer2);
	hav::packet_queue_init(&mAudioBuffer2);
	hav::packet_queue_start(&mVideoBuffer2);
	hav::packet_queue_start(&mAudioBuffer2);

	
	
 	if(!initializeAudioEncoder(AV_CODEC_ID_AAC,0,0,1536000,48000))
 		printf("initializeAudioEncoder EEOR\n");
	if(!initializeVideoDecoder(AV_CODEC_ID_H264))
		printf("initializeVideoDecoder ERROR\n");
	if(!initializeAudioDecoder(AV_CODEC_ID_AAC))
		printf("initializeAudioDecoder EEORR\n");
	if(inp_flag == 0){
		initializeVideoCapture();
		if(!initializeVideoEncoder(AV_CODEC_ID_H264, 640, 480, 200000, 30))
			printf("initializeVideoEncoder ERROR\n");
	}	
	else if(inp_flag == 1){
		initializeMediaFileDecoderEncoder();
		if(!initializeVideoEncoder(AV_CODEC_ID_H264, 1280, 720, 1634000, 30))
			printf("initializeVideoEncoder ERROR\n");
	}
	
	

}

AVPacket* FFMPEG::SoundCapture()
{
	float t, tincr;
	AVPacket pkt;

	av_init_packet(&pkt);
	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	float phase=0;
	float phaseAdder=0;
	int sampleRate=44100;
	int bufferSize= 1024;
	int nChannels=1;
	float volume=0.1;
	float output2[1024]={0};
	{
		float targetFrequency = 2000.0f *10;//* ballstate.xt[2];
		float phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
		phaseAdder = 0.05f * phaseAdder + 0.05f * phaseAdderTarget;
 		for (int i = 0; i < bufferSize; i++){
 			phase += phaseAdder;
			//printf("INP:%f\n",phaseAdder );
 			float sample = sin(phase);
			output2[i*nChannels] = sample * volume ;
			
 			//output[i*nChannels + 1] = sample * volume ;
 		}

	}

	uint8_t fdata;
	uint8_t sdata;
	int16_t ttdata;
	for (int j = 0; j < bufferSize*2; j+=2) 
	{
		ttdata = (int16_t)(output2[j/2]*65535);
		fdata = (uint8_t) ((ttdata & 0xff00) >> 8);
		sdata = (uint8_t) ((ttdata & 0x00ff) >> 0);
		audioFrame->data[0][j] = sdata;
		audioFrame->data[0][j+1] = fdata;
	}


	audioFrame->pts= apts++;
	//audioFrame->nb_samples = 1023;
	int got_output;
	
	int64_t temptime = av_gettime()+timeoffset;

	int ret = avcodec_encode_audio2(mAudioEncoderContext, &pkt, audioFrame, &got_output);
	if (ret < 0) 	fprintf(stderr, "Error encoding audio frame\n");
	else
	{
		hav::packet_queue_put(&mAudioEncodeBuffer, &pkt, temptime);
		//av_free_packet(&pkt);
	}
	return &pkt;
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
int VideoEncodeThread(void* data1)
{
	
		FFMPEG* app = (FFMPEG*)data1;
		while(1){
			if(app->captureflag1 == true)
			{
				int packet_size1;
				av_init_packet(&app->encodedVideoPacket);
				app->encodedVideoPacket.data = NULL;
				app->encodedVideoPacket.size = 0;
				int got_packet1;
				
				app->pictYUV->pts = app->nextPTS();
				int res1 = avcodec_encode_video2(app->mVideoEncoderContext, &app->encodedVideoPacket, app->pictYUV, &got_packet1);
				
				//	int16_t time_stamp1 = av_gettime()+timeoffset;

				if(res1 == 0 && got_packet1 > 0){
					hav::packet_queue_put(&app->mVideoEncodeBuffer, &app->encodedVideoPacket, app->Capture_time);
					packet_size1 = app->encodedVideoPacket.size;
					//printf("%f \n", (double)app->Capture_time);
				}else{
					packet_size1 = -1;
					av_free_packet(&app->encodedVideoPacket);
				}
				app->captureflag1 = false;
			}
	}
	return 0;
}
AVFrame* FFMPEG::VideoDecode(AVPacket decodedVideoPacket)
{
	AVFrame* aFrame = avcodec_alloc_frame();
	if(decodedVideoPacket.stream_index == AVMEDIA_TYPE_VIDEO) {
		int frameFinished;
		int ps = avcodec_decode_video2(mVideoDecoderContext, aFrame, &frameFinished, &decodedVideoPacket);
		if(frameFinished) {
			return aFrame;
		}
	}
	av_frame_free(&aFrame);
	
	return 0;
}


float FFMPEG::AudioDecode(AVPacket pkt, float* output)
{
	int audio_pkt_size = 0;
	AVFrame frame;
	int len1, data_size = 0;
	int resampled_data_size;
	int64_t time_stamp;
	audio_buf1_size1=0;

	audio_pkt_size = pkt.size;
	{
		while(audio_pkt_size > 0)
		{

			int got_frame = 0;

			len1 = avcodec_decode_audio4(mAudioDecoderContext , &frame, &got_frame, &pkt);

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

			int wanted_nb_samples = 1024;

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
				const uint8_t **in = (const uint8_t **)frame.extended_data;
				uint8_t **out = &audio_buf1;
				int out_count = (int64_t)wanted_nb_samples * audio_tgt.freq / frame.sample_rate + 256;
				int out_size  = av_samples_get_buffer_size(NULL, audio_tgt.channels, frame.nb_samples, audio_tgt.fmt, 0);
				int len2;
				if (out_size < 0) {
					av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
					break;
				}
				//if (wanted_nb_samples != frame.nb_samples)
				{
					if (swr_set_compensation(swr_ctx, (wanted_nb_samples - frame.nb_samples) * audio_tgt.freq / frame.sample_rate,
						wanted_nb_samples * audio_tgt.freq / frame.sample_rate) < 0) {
							av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
							break;
					}
				}
				
				av_fast_malloc(&audio_buf1, &audio_buf1_size1, out_size);
				if (!audio_buf1)
					printf("NULL BUFFER~!!!\n");
				//resampled_data_size = 512 * audio_tgt.channels * av_get_bytes_per_sample(audio_tgt.fmt);

				len2 = swr_convert(swr_ctx, out, out_count, in, frame.nb_samples);

				float* data = (float*)frame.data[0];
				idata = (float*) audio_buf1;
				//*output = *idata;

				//printf("idata : %f\noutput:%f\n", idata[0],output[0]);
				//FILE* aout = fopen("out.txt", "w");
				for(int i=0; i<1024; i++)
				{
					float fdata = idata[i];
					
					if(fdata > 1.0f) fdata = 1.0f;
					if(fdata < -1.0f) fdata = -1.0f;
					
					output[i] = fdata;
					

				}
				
				//fclose(aout);
				
			}
		}
	}
	
	
	return 0;

}

bool FFMPEG::initializeMediaFileDecoderEncoder()
{
	

	hav::packet_queue_init(&mAudioTB);
	hav::packet_queue_start(&mAudioTB);
	hav::packet_queue_init(&mVideoTB);
	hav::packet_queue_start(&mVideoTB);
	

	pFormatCtx = avformat_alloc_context();
	const char FileName[] = "../bin/data/au.avi";
	//const char FileName[] = "../bin/data/Iron.Man.And.Hulk.Heroes.United.2013.720P.BRRIP.H264.AAC-MAJESTiC.mp4";
	//const char FileName[] = "../bin/data/out.mkv";
	av_strlcpy(filename, FileName, 255);

	SDL_CreateThread(Media_Decode_thread, this);
	
	return 0;

}

int Media_Decode_thread(void* app)
{
	FFMPEG *is = (FFMPEG*)app;

	AVFormatContext *pFormatCtx = NULL;
	AVPacket pkt1, *packet = &pkt1;
	int video_index = -1;
	int audio_index = -1;


	is->videoStream=-1;
	is->audioStream=-1;

	if(avformat_open_input(&pFormatCtx, is->filename, NULL, NULL)!=0)
		return -1; // Couldn't open file
	is->pFormatCtx = pFormatCtx;

	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	av_dump_format(pFormatCtx, 0, is->filename, 0);
	av_log_set_level(AV_LOG_ERROR);

	for(int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
			video_index < 0) {
				video_index=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
			audio_index < 0) {
				audio_index=i;
		}
	}
	AVCodecContext *codecCtxV = NULL;
	AVCodec *codecV = NULL;
	codecCtxV = pFormatCtx->streams[video_index]->codec;
	codecV = avcodec_find_decoder(codecCtxV->codec_id);

	if(!codecV || (avcodec_open2(codecCtxV, codecV, NULL) < 0)) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	AVCodecContext *codecCtxA = NULL;
	AVCodec *codecA = NULL;
	codecCtxA = pFormatCtx->streams[audio_index]->codec;
	codecA = avcodec_find_decoder(codecCtxA->codec_id);


	codecA = avcodec_find_decoder(codecCtxA->codec_id);
	if(!codecA || (avcodec_open2(codecCtxA, codecA, NULL) < 0)) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	is->tcnt=0;
	AVFrame* CvtFrame;
	for(;;) {
		if(av_read_frame(pFormatCtx, packet) < 0)
		{
			
 			if(is->pFormatCtx->pb->error == 0) {
 				//SDL_Delay(100); /* no error; wait for user input */
 				continue;
 			} else {
 				break;
			}
		}
		else
			
		if(packet->stream_index == video_index) {

			//packet_queue_put(&is->mVideoTB, packet,0);
			AVFrame* vFrame = avcodec_alloc_frame();
			
			int frameFinished;
			int ps = avcodec_decode_video2(codecCtxV, vFrame, &frameFinished, packet);
			if(frameFinished) {
				int got_packet1;
				av_init_packet(&is->encodedVideoPacket);
				is->encodedVideoPacket.data = NULL;
				is->encodedVideoPacket.size = 0;
				
				//CvtFrame = is->VideoFrameFormatConvert(codecCtxV,vFrame);
				//CvtFrame->pts = is->nextPTS();
				vFrame->pts = is->nextPTS();
 				int res1 = avcodec_encode_video2(is->mVideoEncoderContext, &is->encodedVideoPacket, vFrame, &got_packet1);

				if(res1 == 0 && got_packet1 > 0)
				{
					hav::packet_queue_put(&is->mVideoEncodeBuffer, &is->encodedVideoPacket, is->Capture_time);
				}
				
				
				
			}
			//av_frame_free(&vFrame);
			
		} else if(packet->stream_index == audio_index) {
			//packet_queue_put(&is->mAudioTB, packet,0);
			AVFrame* aFrame = avcodec_alloc_frame();

			int frameFinished;
			int ps = avcodec_decode_audio4(codecCtxA, aFrame, &frameFinished, packet);
			if(frameFinished)
			{
				int got_packet1;
				av_init_packet(&is->encodedAudioPacket);
				is->encodedAudioPacket.data = NULL;
				is->encodedAudioPacket.size = 0;

				int res1 = avcodec_encode_audio2(is->mAudioEncoderContext, &is->encodedAudioPacket, aFrame, &got_packet1);
				if(res1 == 0 && got_packet1 > 0)
				{
					hav::packet_queue_put(&is->mAudioEncodeBuffer, &is->encodedAudioPacket, 0);
				}
			}
			
		} else {
			av_free_packet(packet);
		}
	}
}

AVFrame* FFMPEG::VideoFrameFormatConvert(AVCodecContext *aCtx, AVFrame *aFrame)
{
	if (NULL == aFrame || NULL == aCtx) {
		return NULL;
	}
	SwsContext *imgConvertCtx = NULL;
	AVFrame *yuvFrame = NULL;

	imgConvertCtx = sws_getContext(aCtx->width, aCtx->height, aCtx->pix_fmt, aCtx->width, aCtx->height,AV_PIX_FMT_YUV420P,SWS_BICUBIC,NULL,NULL,NULL);
	if (NULL == imgConvertCtx) {
		fprintf(stderr, "Can not initialize the conversion context\n");
		return NULL;
	}

	yuvFrame = avcodec_alloc_frame();
	if (yuvFrame == NULL) {
		sws_freeContext(imgConvertCtx);
		return NULL;
	}

	avcodec_get_frame_defaults(yuvFrame);

	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, aCtx->width,	aCtx->height);
	uint8_t *buffer = (uint8_t*) av_malloc(numBytes);
	avpicture_fill((AVPicture *)yuvFrame, buffer,AV_PIX_FMT_YUV420P, aCtx->width,aCtx->height);

	sws_scale(imgConvertCtx, aFrame->data, aFrame->linesize, 0, aCtx->height, yuvFrame->data, yuvFrame->linesize);

	sws_freeContext(imgConvertCtx);
	return yuvFrame;
}