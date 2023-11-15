#include "MediaCodec.h"


static AVPacket flush_pkt;

static void packet_queue_init(PacketQueue *q) {
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

	AVPacketList *pkt1;
	if(pkt != &flush_pkt && av_dup_packet(pkt) < 0) {
		return -1;
	}
	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
	return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for(;;) {

		//if(global_video_state->quit) {
		//	ret = -1;
		//	break;
		//}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

static void packet_queue_flush(PacketQueue *q) {
	AVPacketList *pkt, *pkt1;

	SDL_LockMutex(q->mutex);
	for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	SDL_UnlockMutex(q->mutex);
}

CMediaCodec::CMediaCodec(void)
{
	mOwnerType = NotYetDesignated; 
	mFormatContext = NULL;
	mEncoderContext = NULL;
	mDecoderContext = NULL;
	mCapturerCodecContext = NULL;
	mCapturerCodec = NULL;
		
	mEncoder = NULL;
	mDecoder = NULL;

	mSwsContext = NULL;
	mIdxInputVideoStream = -1;
	mIdxInputAudioStream = -1;
}


CMediaCodec::~CMediaCodec(void)
{
	if(mFormatContext != NULL){
		avformat_close_input(&mFormatContext);
	}
	if(mCapturerCodecContext != NULL){
		avcodec_close(mCapturerCodecContext);
	}

	if(mEncoderContext != NULL){
		avcodec_close(mEncoderContext);
	}
	if(mDecoderContext != NULL){
		avcodec_close(mDecoderContext);
	}
	
}
bool CMediaCodec::initializeCapturer()
{
	// Find the first video stream
	for(int i=0; i<mFormatContext->nb_streams; i++){
		if(mFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			mIdxInputVideoStream = i;
			break;
		}
	}
	for(int i=0; i<mFormatContext->nb_streams; i++){
		if(mFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
			mIdxInputAudioStream = i;
			break;
		}
	}
	if(mIdxInputVideoStream == -1){
		return false; // Didn't find a video stream
	}

	mCapturerCodecContext = mFormatContext->streams[mIdxInputVideoStream]->codec;
	mCapturerCodec = avcodec_find_decoder(mCapturerCodecContext->codec_id);
	
	// Open Input decoder
	if(avcodec_open2(mCapturerCodecContext, mCapturerCodec, NULL) < 0){
		return false;
	}

return true;
}

bool CMediaCodec::initializeDecoder(enum AVCodecID codecId, int width, int height)
{
	// Decoder
	
	mDecoder = avcodec_find_decoder(codecId);
	mDecoderContext = avcodec_alloc_context3(mDecoder);
	mDecoderContext->width = width;
	mDecoderContext->height = height;
	mDecoderContext->pix_fmt = AV_PIX_FMT_YUV420P;

	// Open codec
	if(avcodec_open2(mDecoderContext, mDecoder, NULL) < 0){
		printf("fail to open decoder \n");
		return false; // Could not open codec
	}

return true;
}

bool CMediaCodec::initializeEncoder(enum AVCodecID codecId, int width, int height, int gop_size)
{
	// Find the decoder for the video stream
	//AVCodecContext* pInputCodecContext = mFormatContext->streams[mIdxInputVideoStream]->codec;
	//AVCodec* inputCodec = avcodec_find_decoder(pInputCodecContext->codec_id);
	//pInputCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

	// Open Input decoder
	//if(avcodec_open2(pInputCodecContext, inputCodec, NULL) < 0){return -1;}
		
	// encoder
	mEncoder = avcodec_find_encoder(codecId);
	mEncoderContext = avcodec_alloc_context3(mEncoder);
	mEncoderContext->bit_rate = width * height * 4;

	mEncoderContext->width = width;
	mEncoderContext->height = height;
	mEncoderContext->time_base.num = 1;
	mEncoderContext->time_base.den = 25;
	mEncoderContext->gop_size = 10;//gop_size;
	mEncoderContext->max_b_frames = 1;
	mEncoderContext->mb_decision  = 2;
	mEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	if(avcodec_open2(mEncoderContext, mEncoder, NULL) < 0){
		printf("encoder setting failure \n");
		return false;
	}
	
	return true;
}

int CMediaCodec::encodeToPacket(AVFrame* aFrame, AVPacket& encoded_packet)
{
	int packet_size;
	av_init_packet(&encoded_packet);
	encoded_packet.data = NULL;
	encoded_packet.size = 0;

	int got_packet = -1;
	AVFrame* yuvFrame = createFrameConvertedToYUV420(mCapturerCodecContext, aFrame);
	int res = avcodec_encode_video2(mEncoderContext, &encoded_packet, yuvFrame, &got_packet);

	if(res == 0 && got_packet > 0){
		packet_size = encoded_packet.size;
		//printf("%d \n", packet_size);
	}else{
		packet_size = -1;
		av_free_packet(&encoded_packet);
	}
	av_frame_free(&yuvFrame);
return packet_size;
}

AVFrame* CMediaCodec::decodeToFrame(AVPacket packet)
{
	AVFrame* aFrame = avcodec_alloc_frame();

	if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
	
		int frameFinished;
		int ps = avcodec_decode_video2(mDecoderContext, aFrame, &frameFinished, &packet);
		if(frameFinished) {
			return aFrame;
		}
	}

	av_frame_free(&aFrame);
	return NULL;
}

AVFrame* CMediaCodec::getFrame()
{
	
	AVFrame* aFrame = NULL; 
		
	AVPacket packet;
	aFrame = avcodec_alloc_frame();
	
	for(;;) {
		if(av_read_frame(mFormatContext, &packet) >= 0){
			if(packet.stream_index == mIdxInputVideoStream) {
				int frameFinished = -1;
				int aa = avcodec_decode_video2(mCapturerCodecContext, aFrame, &frameFinished, &packet);
				if(frameFinished){
					break;
				}		
			}
		}
	}	
	av_free_packet(&packet);
		//av_free(aFrame);
return aFrame;
}

void CMediaCodec::overlayImageFrame(AVFrame* aFrame, SDL_Surface* screen)
{
	
	// Dncoder
	AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->width = 640;
	pCodecCtx->height = 480;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	//pCodecCtx->pix_fmt = AV_PIX_FMT_BGR24;

	
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return; // Could not open codec

	
	SDL_Overlay* bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY,screen);
	struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL, NULL,NULL);
	

	SDL_LockYUVOverlay(bmp);

	AVPicture pict;
	pict.data[0] = bmp->pixels[0];		pict.data[1] = bmp->pixels[2];		pict.data[2] = bmp->pixels[1];
	pict.linesize[0] = bmp->pitches[0]; pict.linesize[1] = bmp->pitches[2]; pict.linesize[2] = bmp->pitches[1];

	// Convert the image into YUV format that SDL uses
	int n = sws_scale(sws_ctx, (uint8_t const * const *)aFrame->data, aFrame->linesize, 0, pCodecCtx->height,pict.data,pict.linesize);

	SDL_UnlockYUVOverlay(bmp);

	SDL_Rect rect;
	rect.x = 0;	rect.y = 0;
	rect.w = pCodecCtx->width; rect.h = pCodecCtx->height;

	SDL_DisplayYUVOverlay(bmp, &rect);

	sws_freeContext(sws_ctx);
	SDL_FreeYUVOverlay(bmp);
	av_free(pCodec);
	av_free(pCodecCtx);
	avcodec_close(pCodecCtx);
}

bool CMediaCodec::initialize(eFFMpegMediaClassOwnerType type, const char* deviceName)
{

	// registration process
	
	if(type == MediaStreamServer){
		mOwnerType = MediaStreamServer;
		
		av_register_all(); 
		avdevice_register_all(); 
		avcodec_register_all();
		avformat_network_init();

		mFormatContext = avformat_alloc_context();
		
		AVDictionary* options = NULL;
		av_dict_set(&options,"list_devices","false",0);  // Set "true" if you want to show the list of capturing devices
		av_dict_set(&options, "framerate", "30", 0); 
		//av_dict_set(&options, "pixel_format", "yuv420p", 0);
		//av_dict_set(&options, "video_size", "960x720", 0);    
		av_dict_set(&options, "video_size", "640x480", 0);    
		AVInputFormat *iformat = av_find_input_format("dshow");

		// open input file, and allocate format context 
		if (avformat_open_input(&mFormatContext, deviceName, iformat, &options) < 0) {
			fprintf(stderr, "Could not open source file %s\n", deviceName);
			return false;
		}

		// retrieve stream information 
		if (avformat_find_stream_info(mFormatContext, NULL) < 0) {
			fprintf(stderr, "Could not find stream information\n");
			return false;
		}

		// ?? to show codec information for debugging
		av_dump_format(mFormatContext, 0, deviceName, false);
		return true;

	}else if(type == MediaStreamClient){
		av_register_all(); 
		avdevice_register_all(); 
		avcodec_register_all();
		avformat_network_init();
		mOwnerType = MediaStreamClient;
		return true;
	}else{
		return false;
	}
	
	return true;
}


void CMediaCodec::initializeFormatContext(AVFormatContext* pFormatCtx, const char* deviceName)
{
	const char inputFormatName[] ="dshow";
	AVDictionary* options = NULL;

	av_dict_set(&options,"list_devices","false",0);  // Set "true" if you want to show the list of capturing devices
	av_dict_set(&options, "framerate", "25", 0); 
	AVInputFormat *iformat = av_find_input_format(inputFormatName);

	// open input file, and allocate format context 
	if (avformat_open_input(&pFormatCtx, deviceName, iformat, &options) < 0) {
		fprintf(stderr, "Could not open source file %s\n", deviceName);
		exit(1);
	}

	// retrieve stream information 
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	// ?? to show codec information for debugging
	av_dump_format(pFormatCtx, 0, deviceName, false);
}


AVFrame* CMediaCodec::createFrameConvertedToYUV420(AVCodecContext *aCtx, AVFrame *aFrame)
{
	if (NULL == aFrame || NULL == aCtx) {
		return NULL;
	}
	SwsContext *imgConvertCtx = NULL;
	AVFrame *yuvFrame = NULL;

	imgConvertCtx = sws_getContext(aCtx->width, aCtx->height, aCtx->pix_fmt, aCtx->width, aCtx->height,AV_PIX_FMT_YUV420P,SWS_BICUBIC,NULL,NULL,NULL);
	if (NULL == imgConvertCtx) {
		printf("\nCan not initialize the conversion context\n");
		return NULL;
	}

	yuvFrame = avcodec_alloc_frame();
	if (yuvFrame == NULL) {
		sws_freeContext(imgConvertCtx);
		return NULL;
	}

	avcodec_get_frame_defaults(yuvFrame);
	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, aCtx->width,	aCtx->height);
	uint8_t *buffer = new uint8_t[numBytes];
	avpicture_fill((AVPicture *)yuvFrame, buffer,AV_PIX_FMT_YUV420P, aCtx->width,aCtx->height);
	sws_scale(imgConvertCtx, aFrame->data, aFrame->linesize, 0, aCtx->height, yuvFrame->data, yuvFrame->linesize);
	
	delete [] buffer; buffer = NULL;
	sws_freeContext(imgConvertCtx);

	return yuvFrame;
}

int CMediaCodec::encode_thread(void* formatContext) 
{
	
	AVFormatContext* pInputFormatContext = (AVFormatContext*) formatContext;

	// Find the first video stream
	int videoStream = -1;
	for(int i=0; i<pInputFormatContext->nb_streams; i++){
		if(pInputFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	}
	if(videoStream == -1){
		return -1; // Didn't find a video stream
	}

	// Find the decoder for the video stream
	AVCodecContext* pInputCodecContext = pInputFormatContext->streams[videoStream]->codec;
	AVCodec* inputCodec = avcodec_find_decoder(pInputCodecContext->codec_id);
	//pInputCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

	// Open Input decoder
	if(avcodec_open2(pInputCodecContext, inputCodec, NULL) < 0){return -1;}

	AVFrame* pFrame = avcodec_alloc_frame();
	AVPacket packet, packet_encoded;
	int frameFinished = -1;

	// encoder
	AVCodec* pEncoder = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
	AVCodecContext* pEncoderContext = avcodec_alloc_context3(pEncoder);
	pEncoderContext->bit_rate = pInputCodecContext->width * pInputCodecContext->height * 4;
	
	pEncoderContext->width = pInputCodecContext->width;
	pEncoderContext->height = pInputCodecContext->height;
	pEncoderContext->time_base = pInputCodecContext->time_base;
	pEncoderContext->gop_size = 10;
	pEncoderContext->max_b_frames=1;
	pEncoderContext->mb_decision = 2;
	pEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	if(avcodec_open2(pEncoderContext, pEncoder, NULL) < 0){
		printf("encoder setting failure \n");
	}

	// convert picture format for a frame decoded 
	struct SwsContext *sws_ctx = sws_getContext(pInputCodecContext->width,	pInputCodecContext->height,	pInputCodecContext->pix_fmt,
												pEncoderContext->width,		pEncoderContext->height,	pEncoderContext->pix_fmt,
												SWS_BILINEAR,NULL, NULL,NULL);
	AVPacket encoded_packet;

	for(;;) {
		SDL_Delay(5);

		if(av_read_frame(pInputFormatContext, &packet) >= 0){
			if(packet.stream_index == videoStream) {
				int frameFinished = -1;
				int aa = avcodec_decode_video2(pInputCodecContext, pFrame, &frameFinished, &packet);
				if(frameFinished){
					av_init_packet(&encoded_packet);
					encoded_packet.data = NULL;
					encoded_packet.size = 0;

					int got_packet = -1;
					AVFrame* yuvFrame = createFrameConvertedToYUV420(pInputCodecContext, pFrame);
					int po =avcodec_encode_video2(pEncoderContext, &encoded_packet, yuvFrame, &got_packet);
					if(po == 0 && got_packet > 0){
						packet_queue_put(&gVideoQueue, &encoded_packet);	
					}else{
						av_free_packet(&encoded_packet);
					}
					av_frame_free(&yuvFrame);
				}		
			}
			av_free_packet(&packet);
		}
	}	

	av_free(pFrame);

	avcodec_close(pInputCodecContext);

	return 0;
}




int CMediaCodec::decode_thread(void* data) 
{
	// Dncoder
	AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->width = 640;
	pCodecCtx->height = 480;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		 
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1; // Could not open codec
	
	// Allocate video frame
	AVFrame* pFrame = avcodec_alloc_frame();
		
	// Allocate a place to put our YUV image on that screen
	SDL_Surface* screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
	
	if(!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		return -1;
	}
	SDL_Overlay* bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY,screen);
	struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL, NULL,NULL);

	AVPacket packet;

	for(;;){
	
		SDL_Delay(10);

		if(gVideoQueue.nb_packets > 1){
			if(packet_queue_get(&gVideoQueue, &packet, 1) > 0) {
				if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
				
					// Decode video frame
					int frameFinished;
					int ps = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
					printf("packet received =  %d \n", packet.size);
										
					if(frameFinished) {
					
					

						SDL_LockYUVOverlay(bmp);

						AVPicture pict;
						pict.data[0] = bmp->pixels[0];		pict.data[1] = bmp->pixels[2];		pict.data[2] = bmp->pixels[1];
						pict.linesize[0] = bmp->pitches[0]; pict.linesize[1] = bmp->pitches[2]; pict.linesize[2] = bmp->pitches[1];

						// Convert the image into YUV format that SDL uses
						int n = sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,pict.data,pict.linesize);

						SDL_UnlockYUVOverlay(bmp);

						SDL_Rect rect;
						rect.x = 0;	rect.y = 0;
						rect.w = pCodecCtx->width; rect.h = pCodecCtx->height;
					
						SDL_DisplayYUVOverlay(bmp, &rect);
					}
				}
			}
			if(packet.data){
				av_free_packet(&packet);
			}
		}
		// Free the packet that was allocated by av_read_frame
		
	}

	av_free(pFrame);
	avcodec_close(pCodecCtx);

	return 0;
}


