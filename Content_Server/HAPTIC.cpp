#include "HAPTIC.h"


HAPTIC::HAPTIC(void)
{
}


HAPTIC::~HAPTIC(void)
{
}
void HAPTIC::Haptic_Init(int64_t t)
{

	hav::haptic_queue_init(&hq);
	hav::haptic_queue_start(&hq);

	timeoffset = t;

	gCursorRadius = 0.05;
	gBallRadius = 100;
	gBallMass = 100;
	gBallViscosity = 0.001;
	gBallStiffness = 400;

	gBallPosition[0]=0;
	gBallPosition[1]=0;
	gBallPosition[2]=0;

	gBallVelocity[0]=0;
	gBallVelocity[1]=0;
	gBallVelocity[2]=0;

	oldpenetrationDist1 = 0;
	oldpenetrationDist2 = 0;

}



//Client1이 큐브에 접촉했을때 
void HAPTIC::touchScene1(void* pUserData, double* penetrationDist1)
{
	
	double force[3];

	HapticPacket* haptics = static_cast< HapticPacket* >( pUserData );
	
	if (gLastTime == 0) 
		gLastTime = getSystemTime();

	
	const double stiffness = 200.0;

	force[0] = 0; force[1] = 0; force[2] = 0;
	double rpos[3];
	rpos[0] = haptics->val.pos[0] - gBallPosition[0];

	double temppos=(haptics->val.pos[0]-140)*(0.05/300);
	double thisTime = getSystemTime();


	if (gBallPosition[0]-gBallRadius < haptics->val.pos[0])
	{
		double temppenetrationDist;
		double penetrationDist = gBallPosition[0] - gBallRadius -haptics->val.pos[0];
		penetrationDist1 = &penetrationDist;
 		
		if(penetrationDist< -2*gBallRadius)
 			penetrationDist -= gBallRadius*2+penetrationDist;
		
		
		temppenetrationDist = (penetrationDist-140)*(0.05/300);

		gBallPosition[0]=haptics->val.pos[0]-gBallPosition[0]+gBallRadius+gBallPosition[0];
	
		//v = (old position - new position)/(new time - old time)
		//F =  -k*d-c*v
		double dt = thisTime - gLastTime;
		gBallVelocity[0] = (oldpenetrationDist1- temppos)/(dt);
		force[0] = -0.09*penetrationDist-0.7*gBallVelocity[0];
		haptics->val.touch = true;
	
	}
	else
		haptics->val.touch = false;

	gLastTime = thisTime;
	oldpenetrationDist1 = temppos;
	haptics->val.timestamp = av_gettime()+timeoffset;
	haptics->val.Force[0] = force[2];
	haptics->val.Force[1] = force[1];
	haptics->val.Force[2] = force[0];

	
	gBallPosition[1]=0;
	gBallPosition[2]=0;
	haptics->val.ballpos[0] = gBallPosition[0];
	haptics->val.ballpos[1] = gBallPosition[1];
	haptics->val.ballpos[2] = gBallPosition[2];
	hav::haptic_queue_put(&hq,&haptics->val,av_gettime()+timeoffset);
}
//Client2이 큐브에 접촉했을때 
void HAPTIC::touchScene2(void* pUserData, double* penetrationDist2)
{

	double force[3];

	HapticPacket* haptics = static_cast< HapticPacket* >( pUserData );

	if (gLastTime2 == 0) 
		gLastTime2 = getSystemTime();



	const double stiffness = 200.0;

	force[0] = 0; force[1] = 0; force[2] = 0;

	
	double rpos[3];
	rpos[0] = haptics->val.pos[0] - gBallPosition[0];

	double temppos=(haptics->val.pos[0]-140)*(0.05/300);
	double thisTime = getSystemTime();
	
	if (gBallPosition[0]+gBallRadius > haptics->val.pos[0])
	{
		
		double temppenetrationDist;
		
		double penetrationDist = gBallPosition[0] + gBallRadius - haptics->val.pos[0];
		temppenetrationDist = (penetrationDist-140)*(0.05/300);
		if(penetrationDist>gBallRadius*2)
			penetrationDist -= penetrationDist-gBallRadius*2;

		penetrationDist2 = &penetrationDist;
	
		gBallPosition[0]=haptics->val.pos[0]-gBallPosition[0]-gBallRadius+gBallPosition[0];
		
		//v = (old position - new position)/(new time - old time)
		//F =  k*d+c*v	
		double dt = thisTime - gLastTime2;
		gBallVelocity[0] = (oldpenetrationDist2- temppos)/(dt);
		force[0] = 0.09*penetrationDist+0.7*(gBallVelocity[0]);
	
		//haptics->val.touch = true;
	}
	else
	//	haptics->val.touch = false;

	gLastTime2 = thisTime;
	oldpenetrationDist2 = temppos;
	haptics->val.Force[0] = force[2];
	haptics->val.Force[1] = force[1];
	haptics->val.Force[2] = force[0];

	if((gBallPosition[0] >300)||(gBallPosition[0]< -300))
	{
		gBallPosition[0] = 0;
		gBallVelocity[0]= 0;
	}
	haptics->val.ballpos[1] = 0;
	haptics->val.ballpos[2] = 0;

	haptics->val.ballpos[0] = gBallPosition[0];
	haptics->val.ballpos[1] = gBallPosition[1];
	haptics->val.ballpos[2] = gBallPosition[2];
	


}
double HAPTIC::getSystemTime()
{
	static double s_wavelength = 0.0;
	static double s_wavelength_x_high = 0.0;
	static bool s_isFirstTime = true;

	if (s_isFirstTime)
	{
		LARGE_INTEGER l_frequency = { 0 };
		::QueryPerformanceFrequency(&l_frequency);
		s_wavelength = 1.0 / double(l_frequency.LowPart);
		s_wavelength_x_high = s_wavelength * double(MAXDWORD);
		s_isFirstTime = false;
	}

	LARGE_INTEGER l_ticks = { 0 };
	::QueryPerformanceCounter(&l_ticks);

	return s_wavelength_x_high * double(l_ticks.HighPart) +
		s_wavelength * double(l_ticks.LowPart);
}