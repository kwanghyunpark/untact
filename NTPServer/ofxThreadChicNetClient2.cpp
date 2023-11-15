#include "ofxThreadChicNetClient2.h"

ofxThreadChicNetClient2::ofxThreadChicNetClient2(): ofThread()
{
	_pBuf = NULL;
	_pCallBack = NULL;
	_nBufSize = 1920*1080*3+10;
	_lReceiveChannel.clear();
}

ofxThreadChicNetClient2::~ofxThreadChicNetClient2()
{
	if(_pBuf) { delete [] _pBuf; _pBuf = NULL; }
}

bool ofxThreadChicNetClient2::Init(std::string strServerAddr, std::map<int, std::string>& mDevices, std::string strName, std::string strPW, OnCallbackChicNetClient pCallback)
{
	if(pCallback == NULL) return false;

	// memory allocation
	if(_pBuf == NULL) _pBuf = new char[_nBufSize];
	_pCallBack = pCallback;

	_mDevices.clear();
	_mDevices = mDevices;

	return _oChicNetClient.Init(strServerAddr, _mDevices, strName, strPW);
}

bool ofxThreadChicNetClient2::SendData(char* szName, char* pData, int nLen, int nChannel)
{
	uint64 nRemoteID = _oChicNetClient.GetRemoteHostID(szName);
	if(nRemoteID == 0) return false;
	return _oChicNetClient.SendData(nRemoteID, pData, nLen, nChannel);
}

bool ofxThreadChicNetClient2::SendData(uint64 nRemoteID, char* pData, int nLen, int nChannel)
{
	if(nRemoteID == 0) return false;
	return _oChicNetClient.SendData(nRemoteID, pData, nLen, nChannel);
}


void ofxThreadChicNetClient2::AddReceiveChannel(int nChannel)
{
	_lReceiveChannel.push_back(nChannel);
}

void ofxThreadChicNetClient2::RemoveReceiveChannel(int nChannel)
{
	_lReceiveChannel.remove(nChannel);
}


//===================================================================
void ofxThreadChicNetClient2::threadedFunction()
{
	uint32 nReceiveSize = 0;
	uint64 nRemoteUser = 0;
	int readlen = 0;
	int nChannel = 0;

	while(1)
	{
		_oChicNetClient.RunFrame();

		for(std::list<int>::iterator iter = _lReceiveChannel.begin(); iter != _lReceiveChannel.end(); iter++)
		{
			nChannel = *iter;

			_oChicNetClient.IsP2PPacketAvailable(&nReceiveSize, nChannel);
			if(nReceiveSize > 0)
			{
				//lock();
				nRemoteUser = _oChicNetClient.ReadP2PPacket(_pBuf, _nBufSize, &readlen, nChannel);
				_pCallBack(nRemoteUser, _pBuf, readlen, nChannel);
				//unlock();
			}
		}


	}
}