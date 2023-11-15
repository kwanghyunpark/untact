#pragma once


#include <math.h>
#include "windows.h"
#include <stdio.h>
#include "../../CommonCodes/hav_fifo.h"

// struct HapticData{
// 	double pos[3];
// 	double ballpos[3];
// 	double Force[3];
// 	bool touch;
// 	int64_t timestamp;
// 	int size;
// };

union HapticPacket{
	hav::HapticData val;
	unsigned char udata[sizeof(hav::HapticData)];
};

class HAPTIC
{
public:
	HAPTIC(void);
	~HAPTIC(void);

	void Haptic_Init(int64_t);
	
	double gBallPosition[3];
	double gBallVelocity[3];
	double gBallMass;
	double gBallViscosity;
	double gBallStiffness;
	double gCursorRadius;
	double gBallRadius;
	double gLastTime ;
	double gLastTime2 ;
	double gCursorScale;
	
	double oldpenetrationDist1;
	double oldpenetrationDist2;

	void ballToolContact1(double pos[3], double force[3]);
	void ballToolContact2(double pos[3], double force[3]);
	void touchScene1(void* pUserData,double* penetrationDist1);
	void touchScene2(void* pUserData,double* penetrationDist1);



	double getSystemTime();

	int64_t timeoffset;

	hav::HapticQueue hq;
	hav::HapticQueue* gethapticBuffer1(){return &hq;};

};

