//#pragma once

#include <iostream>
#include "Leap.h"
#include "../../CommonCodes/MirrorworldConfig.h"
using namespace Leap;


typedef struct 
{
	Vector fpos[5] ;
	Vector fdir[5];
	float dist[5];
	Vector ppos;
	Vector pdir;
	int fcnt;
}Ldata;

static class SampleListener : public Listener {
	Ldata ldata;
public:
// 	virtual void onInit(const Controller&);
// 	virtual void onConnect(const Controller&);
// 	virtual void onDisconnect(const Controller&);
// 	virtual void onExit(const Controller&);
// 	virtual void onFrame(const Controller&);
// 	virtual void onFocusGained(const Controller&);
// 	virtual void onFocusLost(const Controller&);


	void onInit(const Controller&);
	void onConnect(const Controller&);
	void onDisconnect(const Controller&);
	void onExit(const Controller&);
	void onFrame(const Controller&);
	void onFocusGained(const Controller&);
	void onFocusLost(const Controller&);
	Ldata* GetHandData();
	Vector pos1;
	Vector dir1;
	Vector pos2;
	Vector dir2;
	Vector palmpos;
	Vector palmdir;
	
	int fcnt;

	
	
};

