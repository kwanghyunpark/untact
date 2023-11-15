#include "NTPSynchronizer.h"

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include "stdint.h"

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/time.h>
};

#endif

#define ADDR_SERVER "114.70.63.186"

CNTPSynchronizer* pNTPTask = NULL;

CNTPSynchronizer::CNTPSynchronizer(void)
{
	pNTPTask = this;
	mStopSignal = false;
	mNTPTimestamp = 0;
	mNtpTImeOffset = 0;
	mIsReadyToComputeOffset = false;
}


CNTPSynchronizer::~CNTPSynchronizer(void)
{
}

int64_t CNTPSynchronizer::getNTPTime()
{
	return av_gettime()+mNtpTImeOffset;
}
/*
int64_t CTaskForSyncNetworkTime::getCurrentSystemTimeMicros()
{
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time

	// get the high resolution counter's accuracy
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);
	return (1000000*tick.QuadPart / ticksPerSecond.QuadPart);
}
*/
#define MAX_TEST_DATA 200
#define NTP_CHANNEL_NTP 2000
#define NTP_MAX_TIME 15000000 // 15 sec
#define NTP_TYPE_REQUEST	111
#define NTP_TYPE_ACK		222
#define NTP_TYPE_RESPONSE	333

bool CNTPSynchronizer::runNTPSynchronization()
{
	mIsReadyToComputeOffset = false;
	mCountRecivedData = 0;
	mSumOfOffset = 0.0;

	std::map<int, std::string> device;
	device[0] = "NTPClient";
	if(mNtpTask.Init(ADDR_SERVER, device, "NTPClient", "", onPacketReceivedForClient)){
		mNtpTask.AddReceiveChannel(NTP_CHANNEL_NTP);
		mNtpTask.startThread(true, false);
		printf("NTP time synchronization process is now starting.\n");
	}else{
		return false;
	}

	int64_t start_time = av_gettime();
	
	int32_t seq_no = 0;

	for(;;){

		AVIOContext* avioContext = NULL;
		uint8_t* buffer = NULL;

		int64_t time_stamp = av_gettime();

		if(avio_open_dyn_buf(&avioContext) == 0){ // zero means "no problem", otherwise -1 for indicating error 

			avio_w8(avioContext, '#'); // Magic number
			avio_wb32(avioContext, seq_no);
			avio_wb64(avioContext, time_stamp);
			// -------------------------------
			int size = avio_close_dyn_buf(avioContext, &buffer);
			mNtpTask.SendData("NTPServer", (char*)buffer, size, NTP_CHANNEL_NTP);
			av_free(buffer);
		}
		seq_no ++;

		// for client side
		Sleep(25);

		if(av_gettime() - start_time > NTP_MAX_TIME){

			//mNtpTask.waitForThread(true);
			mNtpTask.stopThread();
			printf("\n Time over, NTP synchronization process is failure.\n");

			return false;
		}

		if(mIsReadyToComputeOffset){
			printf("\n estimated offset: %lld, ntp time: %lld \n", mNtpTImeOffset, getNTPTime());
			break;
		}
	}

	//mNtpTask.waitForThread(true);
	mNtpTask.stopThread();
	
	return true;
}

bool CNTPSynchronizer::onPacketReceivedForClient_private(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	int64_t t2 = av_gettime();
	if(nBufLen < 0) return false;

	unsigned char* packetData = (unsigned char*)pBuf;

	if(nChannel == NTP_CHANNEL_NTP){
		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		char head = avio_r8(avioContext);
		if(head == '#'){

			if(nBufLen == 1+8){
				int64_t time_stamp = avio_rb64(avioContext);
				printf("%c : %lld\n", head, time_stamp);

			}else if(nBufLen == 1+4+8+8+8){
				int32_t seq_no = avio_rb32(avioContext);
				int64_t t0 = avio_rb64(avioContext);
				int64_t t1 = avio_rb64(avioContext);
				int64_t t2 = avio_rb64(avioContext);

				int64_t t3 = av_gettime();

				int64_t roundtrip_delay = ((t3-t0) - (t2-t1));
				int64_t offset = ((t1-t0) + (t2-t3))/2;

				int64_t globaltime = av_gettime() + offset;
				
				if(roundtrip_delay < 800000){ // 80ms
					mSumOfOffset += (double)offset;
					mCountRecivedData++;			
				
					//printf("%d, %lld, %lld, %lld (offset: %f-%f = %f) \n", seq_no, t0, t1, t2, (double)(t3-t0), (double)(t2-t1), (double)((t3-t1)+(t2-t1))/2.0 );
					double avg = (double)mSumOfOffset/(double)mCountRecivedData;
					printf("\r roundtrip time = %f [ms], offset = %f [ms], average offset = %f [ms]", 0.001*roundtrip_delay, 0.001*(double)offset, 0.001*avg);
					
					if(mCountRecivedData >= MAX_TEST_DATA){
						mIsReadyToComputeOffset = true;
						mNtpTImeOffset = (int64_t)(mSumOfOffset/(double)mCountRecivedData);
					}
				}
			}

		}
		av_free(avioContext);
	}

	return false;
}

bool __stdcall CNTPSynchronizer::onPacketReceivedForClient(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	return pNTPTask->onPacketReceivedForClient_private(nRemoteUser, pBuf, nBufLen, nChannel);
}



/*
bool CTaskForSyncNetworkTime::runNTPServer()
{
	std::map<int, std::string> device;
	device[0] = "NTPServer";
	if(mNtpTask.Init(ADDR_SERVER, device, "NTPServer", "", onPacketReceivedForServer)){

		printf("NTP server is ready.\n");
		printf("Now, I'm waiting for a certain client's request.\n");

		mNtpTask.AddReceiveChannel(NTP_CHANNEL_NTP);
		mNtpTask.startThread();
	}else{
		return false;
	}

	int64_t start_time = av_gettime();
	mCount = 0;

	while(!mStopSignal){
		if(av_gettime() - start_time > NTP_MAX_TIME){
			mStopSignal = true;
			printf("Time over, NTP synchronization process is failure.\n");
			mNtpTask.stopThread();
			return false;
		}
		//if(av_gettime() - mTimestampForTimeout > NTP_MAX_TIME);
	}

	mNtpTask.stopThread();

return true;
}
*/

/*
// --------------------------------------------------
// packet format
// header(#) | type(int32) | sequence no (int32) | timestamp(int64) 
// --------------------------------------------------
bool CTaskForSyncNetworkTime::onPacketReceivedForServer_private(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	int64_t time_stamp_receivedEvent = av_gettime();
	if(nBufLen <= 0)return false;

	unsigned char* packetData = (unsigned char*)pBuf;
	if(nChannel == NTP_CHANNEL_NTP){

		AVIOContext* avioContext = avio_alloc_context(packetData, nBufLen, 0, NULL, NULL, NULL, NULL);
		// --------------------------------
		// Read data hear
		// --------------------------------
		char head = avio_r8(avioContext);
		if(head == '#'){
			int32_t type_received = avio_rb32(avioContext);
			int32_t seq_no_received = avio_rb32(avioContext);
			int64_t time_stamp_received = avio_rb64(avioContext);
			// -------------------------------
			//	printf("%c : %lld (%d) \n", head, time_stamp, nBufLen);

			AVIOContext* avioContext2 = NULL;
			uint8_t* buffer = NULL;

			if(avio_open_dyn_buf(&avioContext2) == 0){ // zero means "no problem", otherwise -1 for indicating error 
				avio_w8(avioContext, '#'); // Magic number
				avio_wb32(avioContext, NTP_TYPE_RESPONSE);
				avio_wb32(avioContext, seq_no_received);
				avio_wb64(avioContext, time_stamp_received);
				avio_wb64(avioContext, time_stamp_receivedEvent);
				avio_wb64(avioContext, av_gettime());
				// -------------------------------
				int size = avio_close_dyn_buf(avioContext, &buffer);
				mNtpTask.SendData("NTPServer", (char*)buffer, size, NTP_CHANNEL_NTP);
				av_free(buffer);
			}
		}
		av_free(avioContext);
		return true;
	}

	return false;
}
*/


/*
bool __stdcall CTaskForSyncNetworkTime::onPacketReceivedForServer(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	return pNTPTask->onPacketReceivedForServer_private(nRemoteUser, pBuf, nBufLen, nChannel);
}
*/
