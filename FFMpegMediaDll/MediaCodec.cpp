#include "MediaCodec.h"


/*
static AVPacket flush_pkt;

FFMPEGMEDIA_API void packet_queue_init(PacketQueue *q) {
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}

FFMPEGMEDIA_API int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

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

FFMPEGMEDIA_API int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
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

FFMPEGMEDIA_API void packet_queue_flush(PacketQueue *q) {
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
*/

CMediaCodec::CMediaCodec(void)
{
	mOwnerType = NotYetDesignated; 
	mFormatContext = NULL;

	mVideoEncoderContext = NULL;
	mVideoDecoderContext = NULL;
	mVideoCapturerCodecContext = NULL;
	mVideoCapturerCodec = NULL;
	mVideoEncoder = NULL;
	mVideoDecoder = NULL;
	mIdxInputVideoStream = -1;
	mSwsContext = NULL;

	mAudioEncoderContext = NULL;
	mAudioDecoderContext = NULL;
	mAudioCapturerCodecContext = NULL;
	mAudioCapturerCodec = NULL;
	mAudioEncoder = NULL;
	mAudioDecoder = NULL;
	mIdxInputAudioStream = -1;

}


CMediaCodec::~CMediaCodec(void)
{
	if(mFormatContext != NULL){ avformat_close_input(&mFormatContext);}
	
	if(mVideoCapturerCodecContext != NULL){	avcodec_close(mVideoCapturerCodecContext);}
	if(mVideoEncoderContext != NULL){avcodec_close(mVideoEncoderContext);}
	if(mVideoDecoderContext != NULL){avcodec_close(mVideoDecoderContext);}
	
	if(mAudioCapturerCodecContext != NULL){	avcodec_close(mAudioCapturerCodecContext);}
	if(mAudioEncoderContext != NULL){avcodec_close(mAudioEncoderContext);}
	if(mAudioDecoderContext != NULL){avcodec_close(mAudioDecoderContext);}
}


bool CMediaCodec::initialize(eFFMpegMediaClassOwnerType type, const char* deviceName, enum AVCodecID codecId, int width, int height, int bitrate, int gop_size, int64_t offset)
{
	// registration process
	av_register_all(); 
	avdevice_register_all(); 
	avcodec_register_all();
	avformat_network_init();

	mNTPOffset = offset;

	if(type == MediaStreamServer){
		mOwnerType = MediaStreamServer;

		mFormatContext = avformat_alloc_context();
		char captureSize[100];
		sprintf(captureSize, "%dx%d", width, height);

		AVDictionary* options = NULL;
		// for video
		av_dict_set(&options,"list_devices","false",0);  // Set "true" if you want to show the list of capturing devices
		av_dict_set(&options, "framerate", "25", 0); 
		//av_dict_set(&options, "pixel_format", "yuv420p", 0);
		av_dict_set(&options, "video_size", captureSize, 0); 
		
		// for audio
		av_dict_set(&options, "samle_rate", "44100", 0);
		av_dict_set(&options, "channels","1",0);
		av_dict_set(&options, "audio_buffer_size", "10", 0);
		

		AVInputFormat *iformat = av_find_input_format("dshow");

		// open input file, and allocate format context 
		if (avformat_open_input(&mFormatContext, deviceName, iformat, &options) < 0) {
			fprintf(stderr, "Could not initialize video/audio devices  %s\n", deviceName);
			return false;
		}

		// retrieve stream information 
		if (avformat_find_stream_info(mFormatContext, NULL) < 0) {
			fprintf(stderr, "Could not find stream information\n");
			return false;
		}

		// ?? to show codec information for debugging
		av_dump_format(mFormatContext, 0, deviceName, false);

		if(false == initializeVideoCapturer()){
			fprintf(stderr, "Could not initialize capturer context \n");
			return false;
		}

		if(false == initializeVideoEncoder(AV_CODEC_ID_MPEG1VIDEO, width, height, bitrate, gop_size)){
			fprintf(stderr, "Could not initialize encoder context \n");
			return false;
		}

		if(false == initializeVideoDecoder(AV_CODEC_ID_MPEG1VIDEO)){
			fprintf(stderr, "Could not initialize decoder context \n");
			return false;
		}

		if(false == initializeAudioCapturer()){
			fprintf(stderr, "Could not initialize capturer context \n");
			return false;
		}

		return true;

	}else if(type == MediaStreamClient){

		mOwnerType = MediaStreamClient;

		if(false == initializeVideoDecoder(AV_CODEC_ID_MPEG1VIDEO)){
			fprintf(stderr, "Could not initialize decoder context \n");
			return false;
		}
		return true;
	}

	return false;
}


bool CMediaCodec::initializeVideoCapturer()
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (initializeCaturer) does not work for CLIENT module \n");
		return false;
	}

	// Find the first video stream
	for(int i=0; i<mFormatContext->nb_streams; i++){
		if(mFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			mIdxInputVideoStream = i;
			break;
		}
	}
	
	if(mIdxInputVideoStream == -1){
		return false; // Didn't find a video stream
	}

	mVideoCapturerCodecContext = mFormatContext->streams[mIdxInputVideoStream]->codec;
	mVideoCapturerCodec = avcodec_find_decoder(mVideoCapturerCodecContext->codec_id);
	
	// Open Input decoder
	if(avcodec_open2(mVideoCapturerCodecContext, mVideoCapturerCodec, NULL) < 0){
		return false;
	}

return true;
}


bool CMediaCodec::initializeAudioCapturer()
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (initializeCaturer) does not work for CLIENT module \n");
		return false;
	}

	// Find the first audio stream
	for(int i=0; i<mFormatContext->nb_streams; i++){
		if(mFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
			mIdxInputAudioStream = i;
			break;
		}
	}

	if(mIdxInputAudioStream == -1){
		return false; // Didn't find a video stream
	}

	mAudioCapturerCodecContext = mFormatContext->streams[mIdxInputAudioStream]->codec;
	mAudioCapturerCodec = avcodec_find_decoder(mAudioCapturerCodecContext->codec_id);

	// Open Input decoder
	if(avcodec_open2(mAudioCapturerCodecContext, mAudioCapturerCodec, NULL) < 0){
		return false;
	}

	return true;
}


bool CMediaCodec::initializeVideoDecoder(enum AVCodecID codecId)
{
	// Decoder
	mVideoDecoder = avcodec_find_decoder(codecId);
	mVideoDecoderContext = avcodec_alloc_context3(mVideoDecoder);
	
	// the following parameters are updated automatically during parsing encoded packets. 
	//mVideoDecoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	//mVideoDecoderContext->pix_fmt = AV_PIX_FMT_BGR24;

	//mVideoDecoderContext->width = width; 
	//mVideoDecoderContext->height = height;

	if(avcodec_open2(mVideoDecoderContext, mVideoDecoder, NULL) < 0){
		fprintf(stderr, "fail to open decoder \n");
		return false; // Could not open codec
	}
return true;
}

bool CMediaCodec::initializeAudioDecoder(enum AVCodecID codecId)
{
	
	// Decoder
	mAudioDecoder = avcodec_find_decoder(codecId);
	mAudioDecoderContext = avcodec_alloc_context3(mAudioDecoder);

	// ----------------------------------
	// setting audio encoder context hear
	// ----------------------------------
	//mAudioDecoderContext->

	// ----------------------------------

	if(avcodec_open2(mAudioDecoderContext, mAudioDecoder, NULL) < 0){
		fprintf(stderr, "fail to open decoder \n");
		return false; // Could not open codec
	}
	
	return true;
}

bool CMediaCodec::initializeVideoEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size)
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (initializeEncoder) does not work for CLIENT module \n");
		return false;
	}

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

bool CMediaCodec::initializeAudioEncoder(enum AVCodecID codecId, int width, int height, int bitRate, int gop_size)
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (initializeEncoder) does not work for CLIENT module \n");
		return false;
	}

	mAudioEncoder = avcodec_find_encoder(codecId);
	mAudioEncoderContext = avcodec_alloc_context3(mAudioEncoder);
	
	// ----------------------------------
	// setting audio encoder context hear
	// ----------------------------------

	// ----------------------------------

	if(avcodec_open2(mVideoEncoderContext, mVideoEncoder, NULL) < 0){
		printf("encoder setting failure \n");
		return false;
	}
	return true;
}

int CMediaCodec::encodeVideoFrameToPacket(AVFrame* aFrame, AVPacket& encoded_packet)
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (encodeFrameToPacket) does not work for CLIENT module \n");
		return -1;
	}
	
	int packet_size;
	av_init_packet(&encoded_packet);
	encoded_packet.data = NULL;
	encoded_packet.size = 0;

	int got_packet = -1;
	AVFrame* yuvFrame = createFrameConvertedToYUV420(mVideoCapturerCodecContext, aFrame);
	int res = avcodec_encode_video2(mVideoEncoderContext, &encoded_packet, yuvFrame, &got_packet);

	if(res == 0 && got_packet > 0){
		packet_size = encoded_packet.size;
		//printf("%d \n", packet_size);
	}else{
		packet_size = -1;
		av_free_packet(&encoded_packet);
	}
	av_freep(&yuvFrame->data[0]);
	av_frame_free(&yuvFrame);
return packet_size;
}

int CMediaCodec::encodeAudioFrameToPacket(AVFrame* aFrame, AVPacket& encoded_packet)
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (encodeFrameToPacket) does not work for CLIENT module \n");
		return -1;
	}
	
	int packet_size;
	/*
	av_init_packet(&encoded_packet);
	encoded_packet.data = NULL;
	encoded_packet.size = 0;

	int got_packet = -1;
	AVFrame* yuvFrame = createFrameConvertedToYUV420(mVideoCapturerCodecContext, aFrame);
	int res = avcodec_encode_video2(mVideoEncoderContext, &encoded_packet, yuvFrame, &got_packet);

	if(res == 0 && got_packet > 0){
		packet_size = encoded_packet.size;
		//printf("%d \n", packet_size);
	}else{
		packet_size = -1;
		av_free_packet(&encoded_packet);
	}

	av_freep(&yuvFrame->data[0]);
	av_frame_free(&yuvFrame);
	*/
	return packet_size;
}

AVFrame* CMediaCodec::decodeVideoPacketToFrame(AVPacket packet)
{
	AVFrame* aFrame = avcodec_alloc_frame();
	if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
		int frameFinished;
		int ps = avcodec_decode_video2(mVideoDecoderContext, aFrame, &frameFinished, &packet);
		if(frameFinished) {
			return aFrame;
		}
	}
	av_frame_free(&aFrame);
return NULL;
}

AVFrame* CMediaCodec::decodeAudioPacketToFrame(AVPacket packet)
{
	AVFrame* aFrame = avcodec_alloc_frame();
	if(packet.stream_index == AVMEDIA_TYPE_AUDIO) {
		int frameFinished;
		int ps = avcodec_decode_audio4(mAudioDecoderContext, aFrame, &frameFinished, &packet);
		if(frameFinished) {
			return aFrame;
		}
	}
	av_frame_free(&aFrame);
	return NULL;
}

AVFrame* CMediaCodec::captureVideoFrame()
{
	if(mOwnerType == MediaStreamClient){
		fprintf(stderr, "The function (captureAnImageFrame) does not work for CLIENT module \n");
		return NULL;
	}

	AVFrame* aFrame = avcodec_alloc_frame(); 
	AVPacket packet;

	for(;;) {
		if(av_read_frame(mFormatContext, &packet) >= 0){
			if(packet.stream_index == mIdxInputVideoStream) {
				int frameFinished = -1;
				int aa = avcodec_decode_video2(mVideoCapturerCodecContext, aFrame, &frameFinished, &packet);
				if(frameFinished){
					break;
				}		
			}
		}
	}	
	av_free_packet(&packet);
	return aFrame;
}


static int sdl_MediaCaptureThread(void* arg)
{
	
	CMediaCodec* codec = (CMediaCodec*)arg;
	int res = codec->captureMediaFrame();
	
	return res;
}

int CMediaCodec::captureMediaFrame()
{
	hav::packet_queue_init(&mVideoBuffer);
	hav::packet_queue_init(&mAudioBuffer);
	hav::packet_queue_start(&mVideoBuffer);
	hav::packet_queue_start(&mAudioBuffer);

	AVPacket encoded_packet;
	for(;;) {

		AVFrame* aFrame = avcodec_alloc_frame(); 
		AVPacket packet;

		if(av_read_frame(mFormatContext, &packet) >= 0){
			
			int64_t time_stamp = av_gettime() + mNTPOffset;

			if(packet.stream_index == AVMEDIA_TYPE_VIDEO) {
				int frameFinished = -1;
				int aa = avcodec_decode_video2(mVideoCapturerCodecContext, aFrame, &frameFinished, &packet);
				if(frameFinished){
					AVPacket encoded_packet;
					if( encodeVideoFrameToPacket(aFrame, encoded_packet) > 0){
						hav::packet_queue_put(&mVideoBuffer, &encoded_packet, time_stamp);
						//mediaCodec->renderVideoFrame(aFrame, gScreen, mediaCodec->getVideoCapturerContext());
					}else{
						av_free_packet(&encoded_packet);	
					}
				}		
			}else if(packet.stream_index == AVMEDIA_TYPE_AUDIO) {
				hav::packet_queue_put(&mAudioBuffer, &packet, time_stamp);
				//int frameFinished = -1;
				//int aa = avcodec_decode_audio4(mAudioCapturerCodecContext, aFrame, &frameFinished, &packet);
				//if(frameFinished){
				//	break;
				//}	
				//printf("%d \n", gAudioQueue.nb_packets);
			}
		}

		av_free_packet(&packet);
		av_free(aFrame);
	}

	hav::packet_queue_destroy(&mVideoBuffer);
	hav::packet_queue_destroy(&mAudioBuffer);

	return 0;
}


void CMediaCodec::startMediaCapture()
{
	SDL_CreateThread(sdl_MediaCaptureThread, this);
}

void CMediaCodec::renderVideoFrame(AVFrame* aFrame, SDL_Surface* screen, AVCodecContext* pCodecCtx, int location_x)
{
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
	rect.x = location_x;	rect.y = 0;
	rect.w = pCodecCtx->width; rect.h = pCodecCtx->height;

	SDL_DisplayYUVOverlay(bmp, &rect);

	sws_freeContext(sws_ctx);
	SDL_FreeYUVOverlay(bmp);
}


void CMediaCodec::renderAudioFrame(AVFrame* aFrame, SDL_Surface* screen, AVCodecContext* pCodecCtx, int location_x)
{
	
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
