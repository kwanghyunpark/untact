#ifndef _OFX_THREADCHICNETCLIENT2_H_
#define _OFX_THREADCHICNETCLIENT2_H_

#include "ofThread.h"
#include "ofxChicNetClient.h"
#include <map>
#include <list>

typedef bool (__stdcall *OnCallbackChicNetClient)(uint64 nRemoteUser, void* pBuf, int nLen, int nReceiveChannel);

class ofxThreadChicNetClient2: public ofThread
{
	ofxChicNetClient			_oChicNetClient;
	std::map<int, std::string>	_mDevices;
	std::list<int>				_lReceiveChannel;

	char*						_pBuf;
	int							_nBufSize;

	OnCallbackChicNetClient		_pCallBack;

public:
	ofxThreadChicNetClient2();
	virtual ~ofxThreadChicNetClient2();

	bool	Init(std::string strServerAddr, std::map<int, std::string>& mDevices, std::string strName, std::string strPW, OnCallbackChicNetClient pCallback);
	bool	SendData(char* szName, char* pData, int nLen, int nChannel);
	bool	SendData(uint64 nRemoteID, char* pData, int nLen, int nChannel);

	void	AddReceiveChannel(int nChannel);
	void	RemoveReceiveChannel(int nChannel);
	int		RecieveChannelCount() { return _lReceiveChannel.size(); }

	int		GetBufSize() { return _nBufSize; }
	void	SetBufSize(int nBufSize) { _nBufSize = nBufSize; }

protected:
	virtual void threadedFunction();
};

#endif