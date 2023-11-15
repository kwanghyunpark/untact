#include "stdio.h"
#include "ofxThreadChicNetClient2.h"

#define ADDR_SERVER "114.70.63.186"


#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include "stdint.h"
#endif

#include <windows.h>


int64_t av_gettime(void)
{
//#if HAVE_GETTIMEOFDAY
//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
//#elif HAVE_GETSYSTEMTIMEASFILETIME
	FILETIME ft;
	int64_t t;
	GetSystemTimeAsFileTime(&ft);
	t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
	return t / 10 - 11644473600000000; /* Jan 1, 1601 */
//#else
//	return -1;
//#endif
}

int64_t getCurrentSystemTimeMicros()
{
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time

	// get the high resolution counter's accuracy
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);
	return (1000000*tick.QuadPart / ticksPerSecond.QuadPart);
}

int wb_int64(int64_t time, unsigned char buf[]) // read byte from int64 in the form of the big endian
{
	for(int i=0; i<8; i++){
		buf[i] = (unsigned char) ((time >> (7-i)*8) & 0x00000000000000ff);
	}
	return 8;
}

int wb_int32(int32_t time, unsigned char buf[]) // read byte from int64 in the form of the big endian
{
	for(int i=0; i<4; i++){
		buf[i] = (unsigned char) ((time >> (3-i)*8) & 0x000000ff);
	}
	return 4;
}

int wb_int16(int16_t time, unsigned char buf[]) // read byte from int64 in the form of the big endian
{
	buf[0] = (unsigned char) ((time >> 8) & 0x00ff);
	buf[1] = (unsigned char) ((time >> 0) & 0x00ff);
	return 2;
}

int wb_int8(int8_t time, unsigned char buf[]) // read byte from int64 in the form of the big endian
{
	buf[0] = (unsigned char) time;
	return 1;
}

int8_t rb_int8(unsigned char buf[], int* size) // read byte from int64 in the form of the big endian
{
	*size += 1;
	return (int8_t)buf[0];
}

int16_t rb_int16(unsigned char buf[], int* size) // read byte from int64 in the form of the big endian
{
	int16_t data;
	data  = ((int16_t)rb_int8(&buf[0], size) << 8) & 0xff00;
	data |= ((int16_t)rb_int8(&buf[1], size) << 0) & 0x00ff;
	return data;
}

int32_t rb_int32(unsigned char buf[], int* size) // read byte from int64 in the form of the big endian
{
	int32_t data;
	data  = ((int32_t)rb_int16(&buf[0], size) << 16) & 0xffff0000;
	data |= ((int32_t)rb_int16(&buf[2], size) << 0)  & 0x0000ffff;
	return data;
}

int64_t rb_int64(unsigned char buf[], int* size) // read byte from int64 in the form of the big endian
{
	int64_t data;
	data  = ((int64_t)rb_int32(&buf[0], size) << 32) & 0xffffffff00000000;
	data |= ((int64_t)rb_int32(&buf[4], size) << 0) & 0x00000000ffffffff;
	return data;
}



#define NTP_CHANNEL 2000

ofxThreadChicNetClient2* pServer= NULL;

static bool __stdcall onPacketReceived(uint64 nRemoteUser, void* pBuf, int nBufLen, int nChannel)
{
	if(pServer == NULL){ return false;}
	if(nBufLen < 1) {return false;}
	
	int64_t t2 = av_gettime();
	
	unsigned char* packetData = (unsigned char*)pBuf;

	if(nChannel == NTP_CHANNEL){
		int size = 0;
		if(rb_int8(packetData, &size) == '#'){
			if(nBufLen == 1+4+8){
				int32_t seq_no   = rb_int32(&packetData[size], &size);
				int64_t time_stamp_rcv = rb_int64(&packetData[size], &size);
				
				int size = 0;
				unsigned char buf[200];
				size += wb_int8('#', &buf[size]);				// header 
				size += wb_int32(seq_no, &buf[size]);			// seq_no 
				size += wb_int64(time_stamp_rcv, &buf[size]);	// 
				size += wb_int64(t2, &buf[size]);	// 
				size += wb_int64(av_gettime(), &buf[size]);	// 
				pServer->SendData(nRemoteUser, (char*)buf, size, nChannel);

				if(seq_no == 1){
					printf("remote user %d is requesting time sync process.\n", nRemoteUser);
				}
				
			}else if(nBufLen == 1){
				unsigned char buf[100];
				int size = 0;
				
				size += wb_int8('#', &buf[size]);  //header 
				size += wb_int64(av_gettime(), &buf[size]);
				pServer->SendData(nRemoteUser, (char*)buf, size, nChannel);
			}
			
		}
	}
	return false;
}


//========================================================================
int main(int argc, char* argv[])
{
	
	ofxThreadChicNetClient2 ntpServer;
	pServer = &ntpServer;

	std::map<int, std::string> device;
	device[0] = "NTPServer";
	if(ntpServer.Init(ADDR_SERVER, device, "NTPServer", "", onPacketReceived)){

		printf("NTP server is ready.\n");
		printf("Now, I'm waiting for a certain client's request.\n");

		ntpServer.AddReceiveChannel(NTP_CHANNEL);
		ntpServer.startThread();
	}else{
		return false;
	}

	for(;;){

	}

	ntpServer.stopThread();
	
	
	return 0;
}
