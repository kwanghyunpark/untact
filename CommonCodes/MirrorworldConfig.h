#pragma once 

#define SERVER_IP_ADDRESS  "114.70.63.186"

#define VIDEO_CHNNEL 1000
#define AUDIO_CHNNEL 1001
#define HAPTIC_CHNNEL 1002

#define WIDTH  640
#define HEIGHT 480
#define CHANNEL 3

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

#define SERIAL_WAIT 20
#define SOUND_WAIT 60
#define SPHERE_SIZE 200
#define SPHERE_JUMP 300
#define FINGER_SPHERE_SIZE 10
/*
struct Haptic_Sensors{
	float x1;
	float x2;
	float x3;
};
	*/
struct Haptic_Effectors{
	float force;
};

struct HapticData{
	struct{
		float fingerpos[5][3];
		float fingerdir[5][3];
		
		float palmpos[3];
		float palmdir[3];
		int fingercnt;


	}sensors;
	
	struct{
		int vib1;
		int vib2;
		float dist1;
		float dist2;
	}effectors;
};

union HapticPacket{
	HapticData val;
	unsigned char udata[sizeof(HapticData)];
};

struct AudioData{
	float left[256];
	//float right[256];
};
union AudioPacket{
	AudioData val;
	unsigned char udata[sizeof(AudioData)];
};

