#pragma once

#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <vector>
#include <map>
#include <list>
#include <hdl/hdl.h>
#include <hdlu/hdlu.h>
#include <mmsystem.h>
#include "hav_fifo.h"

#define HAPTICMODE_VIRTUAL 0
#define HAPTICMODE_REAL 1

struct aBallModelParams
{
	double theta;  // shooter's elevation angle
	double m;      // mass
	double c;      // damping coefficient
	double v_init; // initial speed of ball
};

struct aState
{
	double dt;
	double xt[3];
	double vt[3];

	void reset()
	{
		for(int i=0; i<3; i++){
			xt[i] = 0.0;
			vt[i] = 0.0;
		}
		dt = 0.001;
	};
	void update(double _xt[], double _vt[], double _dt)
	{
		for(int i=0; i<3; i++){
			xt[i] = _xt[i];
			vt[i] = _vt[i];
		}
		dt = _dt;
	}
};

struct aHapticDevice
{
	int id;
	HDLDeviceHandle handle;
	double workspaceDims[6];
	double cursorPos[3];
	double transformMat[16];
	double force[3];
	int   button;
	//GLfloat* color;

	aState state;
	aState state_old;
};

typedef std::vector<aHapticDevice> HDContainer;

class HapticWorldModelBase
{
protected: 

public:
	HapticWorldModelBase(){};
	~HapticWorldModelBase(){};

	//virtual void initialize(std::vector<int> haptic_id) = NULL;
	//virtual void finalize() = NULL;
	virtual void doObjectDynamics(HDContainer& haptics, double dt) = NULL;
	virtual void doContactProcessing(aHapticDevice* haptic, double dt) = NULL;
};


class HapticWorldModel : public HapticWorldModelBase
{
private:
	aState ballState;
	aState ballState_old;
	aBallModelParams ballModelParams;
	bool fIsBallCatched;
	bool fIsAutoThrowMode;

	std::vector<int> haptic_id;
	int numHapticObjects;
public:
	HapticWorldModel(std::vector<int> haptic_id);
	~HapticWorldModel();

	void initialize(std::vector<int> haptic_id);
	void finalize();
	void doObjectDynamics(HDContainer& haptics,double dt);
	void doContactProcessing(aHapticDevice* haptic, double dt);
	aState& getBallState(){return ballState; };
	
	void forcedSetBallState(double xt[], double vt[], double dt)
	{
		ballState.update(xt, vt, dt);
	}
	void setBallState(aState state)
	{
		ballState_old.reset();
		ballState.update(state.xt, state.vt, state.dt);
		fIsBallCatched = false;
	}
	void setBallModelParams(double th, double mass, double damping_coeff, double speed_init)
	{
		ballModelParams.theta = th;
		ballModelParams.m = mass;
		ballModelParams.c = damping_coeff;
		ballModelParams.v_init = speed_init;
	};
};


class HapticWorld
{
public:
	HapticWorld(void);
	~HapticWorld(void);

	void setThrowingCommand(int mode){

		HapticWorldModel* model = getHapticWorldModel();

		double theta	= 20.0*3.141592/180.0;
		double mass		= 1.0;
		double damping	= 1.5;
		double v_init	= 12.0;
		if(mode == 1){
			theta = 45.0*3.141592/180.0;
			mass =  10.0;
			damping = 1.5;
			v_init = 7.4;
		}else if(mode == 2){
			theta = 5.0*3.141592/180.0;
			mass =  1.0;
			damping = 1.5;
			v_init = 20.0;
		}
		model->setBallModelParams(theta, mass, damping, v_init);
		
		aState state;
		state.reset();
		state.xt[2] = -5.0;
		state.vt[2] =  v_init*cos(theta);
		state.vt[1] =  v_init*sin(theta);
		model->setBallState(state);
	};

	bool initialize(HapticWorldModel* hapticWorldModel, std::vector<int> device_id, int hapticMode = HAPTICMODE_REAL,  int samplingTime_ms = 1);
	void HapticWorld::exitHandler();
	void initIndexedDevice(int id, const int ndx);
	aHapticDevice* findDevice(int id)
	{
		if(mapDeviceId.find(id) != mapDeviceId.end()){
			return &haptics[mapDeviceId[id]];
		}
		return NULL;
	};
	
	HapticWorldModel* getHapticWorldModel()
	{
		return pWorldModel;
	};

	void synchFromServo();

	hav::HapticQueue* getHapticQueueSender(){return &hapticQueueSender;}; 
	hav::HapticQueue* getHapticQueueReceiver(){return &hapticQueueReceiver;}; 
private:
		
	hav::HapticQueue hapticQueueSender; 
	hav::HapticQueue hapticQueueReceiver; 


	HapticWorldModel* pWorldModel;
	HDContainer haptics;

	std::vector<int> device_id;
	std::map<int, int> mapDeviceId;

	bool selectionDone;
	int numDevices;
	HDLServoOpExitCode touchOp;
	double lasttime;

	// ==================
	// for real haptic
	// callback functions as a member function of this class
	HDLServoOpExitCode getStateCallback();
	HDLServoOpExitCode controlCallback();

	static HDLServoOpExitCode _intraControlCallback(void *data){
		HapticWorld* hapticDevices = static_cast< HapticWorld* >(data);
		return hapticDevices->controlCallback();
	}
	static HDLServoOpExitCode _intraGetStateCallback(void* pUserData)
	{
		HapticWorld* hapticDevices = static_cast< HapticWorld* >(pUserData);
		return hapticDevices->getStateCallback();
	}
	// ------------------


	// ==================
	// for virtual haptic
	bool isVirtualHaptic;
	int samplingTimeForVirtualHaptic;
	UINT idTimerEventForVirtualHaptic;

	void timerEventListenerForVirtualHaptic(UINT wTimerID);

	static void CALLBACK _intraTimerEventListenerForVirtualHaptic(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
	{
		HapticWorld* obj = (HapticWorld*) dwUser;
		obj->timerEventListenerForVirtualHaptic(wTimerID);
	};
	// ------------------
};



