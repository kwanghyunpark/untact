#include <hdl/hdl.h>
#include <hdlu/hdlu.h>

#pragma once
class Falcon
{
public:
	Falcon(void);
	~Falcon(void);

	void Falcon_Init();
	void testHDLError(const char* str);

	void uninit();

	// Get position
	void getPosition(double pos[3]);

	// Get state of device button
	bool isButtonDown();

	// synchFromServo() is called from the application thread when it wants to exchange
	// data with the HapticClass object.  HDAL manages the thread synchronization
	// on behalf of the application.
	void synchFromServo();

	// Get ready state of device.
	bool isDeviceCalibrated();

private:
	// Move data between servo and app variables
	void synch();

	// Calculate contact force with cube
	void cubeContact();

	// Matrix multiply
	void vecMultMatrix(double srcVec[3], double mat[16], double dstVec[3]);


	// Nothing happens until initialization is done
	bool m_inited;

	// Transformation from Device coordinates to Application coordinates
	double m_transformMat[16];

	// Variables used only by servo thread
	double m_positionServo[3];
	bool   m_buttonServo;
	double m_forceServo[3];

	// Variables used only by application thread
	double m_positionApp[3];
	bool   m_buttonApp;

	// Keep track of last face to have contact
	int    m_lastFace;

	// Handle to device
	HDLDeviceHandle m_deviceHandle;

	// Handle to Contact Callback 
	HDLServoOpExitCode m_servoOp;

	// Device workspace dimensions
	double m_workspaceDims[6];

	// Size of cube
	double m_cubeEdgeLength;

	// Stiffness of cube
	double m_cubeStiffness;

	double gBallPosition[3];
	double gBallVelocity[3];
	double gBallMass;
	double gBallViscosity;
	double gBallStiffness;
	double gCursorRadius;
	double gBallRadius;
	double gLastTime ;
	double gCursorScale;
	//GLuint gCursorDisplayList;
	void ballToolContact(double pos[3], double force[3]);

};

