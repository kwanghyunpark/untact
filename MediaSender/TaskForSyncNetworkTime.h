#pragma once
#include "ofxThreadChicNetClient.h"

//#define NTP_SERVER_SIDE 
//#define NTP_CLIENT_SIDE 
//#include <stdio.h>

class CNTPSynchronizer
{
public:
	CNTPSynchronizer(void);
	~CNTPSynchronizer(void);
	//bool runNTPServer();
	bool runNTPSynchronization();
	int64_t getNTPTime();
	int64_t getOffset(){	return mNtpTImeOffset;}

private:
	int mCountRecivedData;
	int64_t mNTPTimestamp;
	int64_t mNtpTImeOffset;
	double mSumOfOffset;
	bool mIsReadyToComputeOffset;
	
	bool mStopSignal;
	int64_t mTimestampForTimeout;
	ofxThreadChicNetClient mNtpTask;
	
	//static bool __stdcall onPacketReceivedForServer(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);
	static bool __stdcall onPacketReceivedForClient(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);

	//bool onPacketReceivedForServer_private(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);
	bool onPacketReceivedForClient_private(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel);

	int64_t CNTPSynchronizer::getCurrentSystemTimeMicros();
};

