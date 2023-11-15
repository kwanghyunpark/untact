#include "HapticDevices.h"
# include "stdint.h"

int64_t getmicrotime(void)
{
	FILETIME ft;
	int64_t t;
	GetSystemTimeAsFileTime(&ft);
	t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
	return t / 10 - 11644473600000000; /* Jan 1, 1601 */
}


// Check HDAL error and report with user-defined string
void testHDLError(const char* str)
{
	HDLError err = HDL_NO_ERROR;
	err = hdlGetError();
	if (err != HDL_NO_ERROR)
	{
		char msg[200];
		sprintf(msg, "HDAL ERROR %d", err);
		MessageBox(NULL, str, msg, MB_OK);
		exit(1);
	}
}


// This is the callback function that is exercised at servo rate (1KHz)
//HDLServoOpExitCode InteractionWorld::doContactOperation(void *pUserData)
//{
//	return HDL_SERVOOP_CONTINUE;
//}

void HapticWorld::timerEventListenerForVirtualHaptic(UINT wTimerID)
{
	if(wTimerID != idTimerEventForVirtualHaptic){
		return;
	}


	if(this->hapticQueueReceiver.nb_packets > 0){
		hav::HapticData hapticdata;
		int64_t time_stamp;
		while(this->hapticQueueReceiver.nb_packets > 10){
			hav::haptic_queue_get(&hapticQueueReceiver, &hapticdata, 1, &time_stamp);
		}

		if(hav::haptic_queue_get(&hapticQueueReceiver, &hapticdata, 1, &time_stamp) < 0) {
			return;
		}
		haptics[0].button = hapticdata.button;
		for(int i=0; i<3; i++){
			haptics[0].force[i] = hapticdata.Force[i];
			haptics[0].cursorPos[i] = hapticdata.pos[i];
		}
	}
	
	double posDC[3] = {0,0,0};
	double posWC[3] = {0,0,0};
	double force[3] = {0,0,0};

	static int64_t t0 = 0;

	int64_t time = getmicrotime();
	double dt = (double)(time-t0)*0.000001;

	for (HDContainer::iterator it = haptics.begin(); it != haptics.end(); ++it){

		((HapticWorldModelBase*)pWorldModel)->doContactProcessing(&(*it), dt);
	}

	((HapticWorldModelBase*)pWorldModel)->doObjectDynamics(haptics, dt);


	//---------추후 더 좋은 방법으로 바꿀 것 -------------
	hav::HapticData hapticdata;
	aState ballstate = pWorldModel->getBallState();
	for(int i=0; i< 3; i++){
		hapticdata.ballpos[i] = ballstate.xt[i];
		hapticdata.pos[i] = ballstate.vt[i];
	}
	hapticdata.timestamp = time;
	hav::haptic_queue_put(&hapticQueueSender, &hapticdata, time);
	//--------------------------------

	t0 = time;
}

void HapticWorld::synchFromServo()
{
	if(isVirtualHaptic){
		return;
	}
	hdlCreateServoOp(_intraGetStateCallback, this, true);
}

// Get all the devices' state information
HDLServoOpExitCode HapticWorld::getStateCallback()
{
	if(isVirtualHaptic){
		return HDL_SERVOOP_EXIT;
	}

	for (HDContainer::iterator it = haptics.begin(); it != haptics.end(); ++it)
	{
		if ((*it).handle != HDL_INVALID_HANDLE)
		{
			aHapticDevice& hd = *it;

			hdlMakeCurrent(hd.handle);
			hdlToolPosition(&(hd.cursorPos[0]));
			
			//printf("%lf\n",hd.cursorPos[0]);
			
			hdlToolButtons(&(hd.button));
			testHDLError("hdlToolPosition");
		}
	}

	return HDL_SERVOOP_EXIT;
}


HDLServoOpExitCode HapticWorld::controlCallback() //(void* pUserData)
{
	static long count = 0;
	if(++count % 10 != 0){
		return HDL_SERVOOP_CONTINUE;
	}	
	
	double posDC[3] = {0,0,0};
	double posWC[3] = {0,0,0};
	double force[3] = {0,0,0};

	static int64_t t0 = 0;

	int64_t time = getmicrotime();
	double dt = (double)(time-t0)*0.000001;
	
	//if(fabs(dt - 0.005) > 0.001) printf("sampling time mismatched %f \n", dt);
	
	// received data
	aHapticDevice remoteHaptic;
	if(this->hapticQueueReceiver.nb_packets > 0){
		hav::HapticData hapticdata;
		int64_t time_stamp;
		while(this->hapticQueueReceiver.nb_packets > 10){
			hav::haptic_queue_get(&hapticQueueReceiver, &hapticdata, 1, &time_stamp);
		}

		if(hav::haptic_queue_get(&hapticQueueReceiver, &hapticdata, 1, &time_stamp) < 0) {
			return HDL_SERVOOP_CONTINUE;
		}

		pWorldModel->forcedSetBallState(hapticdata.ballpos, hapticdata.pos, dt);
		
	}


	if (selectionDone && t0 > 0)
	{
		for (HDContainer::iterator it = haptics.begin(); it != haptics.end(); ++it)
		{
			// Calculate and set forces for each device that has not
			// been uninited.
			if ((*it).handle != HDL_INVALID_HANDLE)
			{
				hdlMakeCurrent((*it).handle);

				hdlToolPosition((*it).cursorPos);
				hdlToolButtons(&(*it).button);
				
				//vecMultMatrix(posDC, (*it).transformMat, posWC);
					
				//printf("%f, %f, %f \n ", posDC[0],posDC[1],posDC[2]);
				
				((HapticWorldModelBase*)pWorldModel)->doContactProcessing(&(*it), dt);
				
				hdlSetToolForce( (*it).force );
			}

		}

		((HapticWorldModelBase*)pWorldModel)->doObjectDynamics(haptics, dt);


		//---------추후 더 좋은 방법으로 바꿀 것 -------------
		hav::HapticData hapticdata;
		for(int i=0; i< 3; i++){
			hapticdata.pos[i] = haptics[0].cursorPos[i];
			hapticdata.Force[i] = haptics[0].force[i];
		}
		hapticdata.button = haptics[0].button;
		hapticdata.timestamp = time;

		hav::haptic_queue_put(&hapticQueueSender, &hapticdata, time);
		//--------------------------------

		//hapticDataList.push_back()
	}

	t0 = time;
	
	return HDL_SERVOOP_CONTINUE;
}


HapticWorld::HapticWorld(void)
{
	touchOp = HDL_INVALID_HANDLE;
	selectionDone = false;
	pWorldModel = NULL;
	isVirtualHaptic = true;

}


HapticWorld::~HapticWorld(void)
{
	exitHandler();
}

void HapticWorld::initIndexedDevice(int id, const int ndx)
{
	// Initialize device by index into (hidden) device list.
	// Note that the application does not have access to list of device
	// serial numbers.  It only knows how many devices are in the list.

	HDLError err = HDL_NO_ERROR;

	aHapticDevice hd;
	hd.id = id;
	hd.handle = hdlInitIndexedDevice(ndx);
	//hd.color = color;
	for (int i = 0; i < 3; i++)
	{
		hd.cursorPos[i] = 0;
		hd.force[i] = 0;

		hd.state.reset();
		hd.state_old.reset();
	}
	char msg[200];
	if (hd.handle < 0){
		MessageBox(NULL, msg, "Indexed Initialization error", MB_OK);
		abort();
	}

	haptics.push_back(hd);

}

/* initHDL() sets up the HDAL connections.
   The main purpose is to associate specific devices with application functions.
   In this case, the functions are colored cursors.
   Option 1: No command line parameters
             Use named sections in hdal.ini.  Requires device SNs
   Option 2: Execution-time selection
             Number of cursors to use is passed on the command line and
             remembered in global variable gNumCursors.
             Prompt the user to press a button on the device to use for
             control of specified cursors.  Uninitialize left-over
             devices if there are any. */
bool HapticWorld::initialize(HapticWorldModel* hapticWorldModel, std::vector<int> device_id, int hapticMode, int samplingTime_ms)
{
	pWorldModel = hapticWorldModel;
	int numRequiredHaptic = device_id.size();
	
	hav::haptic_queue_init(&hapticQueueSender); 
	hav::haptic_queue_start(&hapticQueueSender);
	hav::haptic_queue_init(&hapticQueueReceiver); 
	hav::haptic_queue_start(&hapticQueueReceiver);


	// virtual mode  
	if(hapticMode == HAPTICMODE_VIRTUAL){
		mapDeviceId.clear();
		for (int i = 0; i < numRequiredHaptic; i++){
			mapDeviceId[device_id[i]] = i;
			aHapticDevice hd;
			for (int k = 0; k < 3; k++){
				hd.cursorPos[k] = 0;
				hd.force[k] = 0;
				hd.state.reset();
				hd.state_old.reset();
			}
			haptics.push_back(hd);
		}
		isVirtualHaptic = true;

		samplingTimeForVirtualHaptic = samplingTime_ms;
		idTimerEventForVirtualHaptic = timeSetEvent(samplingTime_ms,1,_intraTimerEventListenerForVirtualHaptic,(DWORD)this,TIME_PERIODIC);

		return true;
	}

	// real mode
	isVirtualHaptic = false;
	numDevices = hdlCountDevices();
	if (numDevices < numRequiredHaptic){
		printf("You requested %d devices, but only %d are connected.\n", numRequiredHaptic, numDevices);
		printf("Continuing with %d devices/cursors.\n", numDevices);
		return false;
	}
	numDevices = numRequiredHaptic;
		
	selectionDone = false;
	mapDeviceId.clear();
	for (int i = 0; i < numDevices; i++){
		mapDeviceId[device_id[i]] = i;
		initIndexedDevice(device_id[i], i);
	}
	hdlStart();
	Sleep(1000);  // Wait for data to stabilize
	selectionDone = true;
	
    const bool bBlocking = false;
	touchOp = hdlCreateServoOp(_intraControlCallback, this, bBlocking);
    testHDLError("hdlCreateServoOp");
return true;
}

void HapticWorld::exitHandler()
{
	if(isVirtualHaptic){
		return;
	}

    if (touchOp != HDL_INVALID_HANDLE){
        hdlDestroyServoOp(touchOp);
        touchOp = HDL_INVALID_HANDLE;
    }
    hdlStop();

    for (HDContainer::iterator it = haptics.begin(); it != haptics.end(); ++it){
        if ((*it).handle != HDL_INVALID_HANDLE)
            hdlUninitDevice((*it).handle);
    }
}

HapticWorldModel::HapticWorldModel(std::vector<int> haptic_id)
{
	initialize(haptic_id);
}

HapticWorldModel::~HapticWorldModel()
{

}

void HapticWorldModel::initialize(std::vector<int> haptic_id)
{
	this->numHapticObjects = haptic_id.size();
	this->haptic_id.clear();
	for(int i=0; i<haptic_id.size(); i++){
		this->haptic_id.push_back(haptic_id[i]);
	}

	ballState.reset();
	ballState_old.reset();
	
	fIsBallCatched = false;
	fIsAutoThrowMode = false;
	setBallModelParams(20.0*3.141592/180.0, 1.0, 1.5, 0.0);
	aState state;
	state.reset();
	state.xt[2] = -5.0;
	setBallState(state);
}

void HapticWorldModel::finalize()
{
	
}

#define MAX(a,b) ((a) < (b) ? (b) : (a))

void HapticWorldModel::doObjectDynamics(HDContainer& haptics, double dt)
{
	static int cnt =0;

	if( !fIsBallCatched ){
		double acc[3];
		double g = 9.8;

		double Fo[3];
		Fo[0] = -ballModelParams.c*ballState.vt[0] - haptics[0].force[0];
		Fo[1] = -ballModelParams.m*g - ballModelParams.c*ballState.vt[1] - haptics[0].force[1];
		Fo[2] = -ballModelParams.c*ballState.vt[2] - haptics[0].force[2];

		for(int i=0; i<3; i++){
			acc[i] = Fo[i]/ballModelParams.m;
			ballState.vt[i] += acc[i]*dt ;
			ballState.xt[i] += ballState.vt[i]*dt;
		}

		
		if(ballState.xt[1] < - 1.0){
			//fIsBallCatched = true;
		}
	}

	/*
	if(cnt++ > 2000){
		aState state;
		state.reset();
		state.xt[2] = -5.0;
		state.vt[2] =  ballModelParams.v_init*cos(ballModelParams.theta);
		state.vt[1] =  ballModelParams.v_init*sin(ballModelParams.theta);
		setBallState(state);
		cnt = 0;
		//fIsBallCatched = false;
	}
	printf("%d \n", cnt);
	//printf("%f, %f, %f \n", ballState.vt[0],ballState.vt[1],ballState.vt[2]);
	*/
}

void HapticWorldModel::doContactProcessing(aHapticDevice* haptic, double dt)
{
	double *pos = &haptic->cursorPos[0]; 
	double *force = &haptic->force[0];
	 
	double radius = 0.11/2.0;
	double stiffness = 800;
	double damper = 15;
		
	for(int i=0; i<3; i++){
		haptic->state.xt[i] = pos[i];
		haptic->state.vt[i] = (haptic->state.xt[i] - haptic->state_old.xt[i])/dt;
		haptic->state.dt = dt;
	}

	// compute haptic force
	for(int i=0; i<3; i++){
		haptic->force[i] = 0;
	}
	if( !fIsBallCatched ){
		double dist_th = 0.05 + 0.05; //r1+ r2
		double diff_a = sqrt( (haptic->state.xt[0] - ballState.xt[0])*(haptic->state.xt[0] - ballState.xt[0])
					        + (haptic->state.xt[1] - ballState.xt[1])*(haptic->state.xt[1] - ballState.xt[1])
					        + (haptic->state.xt[2] - ballState.xt[2])*(haptic->state.xt[2] - ballState.xt[2]));
		
		if(diff_a < dist_th){
			force[0] = fabs(diff_a)*stiffness + damper*ballState.vt[0];
			force[1] = fabs(diff_a)*stiffness + damper*ballState.vt[1];
			force[2] = fabs(diff_a)*stiffness + damper*ballState.vt[2];
		}
		
		haptic->state_old.update(haptic->state.xt, haptic->state.vt, dt);
	
		for(int i=0; i<3; i++){
			haptic->force[i] = force[i];
		}
	}
}