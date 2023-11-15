#include "SampleListener.h"


void SampleListener::onInit(const Controller& controller) {
	std::cout << "Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller) {
	std::cout << "Connected" << std::endl;
	controller.enableGesture(Gesture::TYPE_CIRCLE);
	controller.enableGesture(Gesture::TYPE_KEY_TAP);
	controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
	controller.enableGesture(Gesture::TYPE_SWIPE);

}

void SampleListener::onDisconnect(const Controller& controller) {
	//Note: not dispatched when running in a debugger.
	std::cout << "Disconnected" << std::endl;
}

void SampleListener::onExit(const Controller& controller) {
	std::cout << "Exited" << std::endl;
}



Ldata* SampleListener::GetHandData()
{
	return &ldata;
}

void SampleListener::onFrame(const Controller& controller) {
	// Get the most recent frame and report some basic information
	const Frame frame = controller.frame();
	//   std::cout //<< "Frame id: " << frame.id()
	//             //<< ", timestamp: " << frame.timestamp()
	//             << ", hands: " << frame.hands().count()
	//             << ", fingers: " << frame.fingers().count()
	//             << ", tools: " << frame.tools().count()
	//             << ", gestures: " << frame.gestures().count() << std::endl;
	
	if (!frame.hands().isEmpty()) {
		// Get the first hand
		const Hand hand = frame.hands()[0];

		// Check if the hand has any fingers
		const FingerList fingers = hand.fingers();
		if (!fingers.isEmpty()) {
			// Calculate the hand's average finger tip position
			Vector avgPos;
			ldata.fcnt = fingers.count();
			for (int i = 0; i < fingers.count(); ++i) {
				ldata.fpos[i] = fingers[i].tipPosition();
				ldata.fdir[i] = fingers[i].direction();
// 				ldata.dist[i] = sqrt((0.0f - ldata.fpos[i].x) * (0.0f - ldata.fpos[i].x) 
// 					+ (SPHERE_JUMP - ldata.fpos[i].y) * (SPHERE_JUMP - ldata.fpos[i].y)
// 					+(0.0f - ldata.fpos[i].z) * (0.0f - ldata.fpos[i].z) );
				


				//avgPos += fingers[i].tipPosition();
			}

			Vector tvec;
			
			float tdist;
			for(int i=0; i<fingers.count(); i++)
			{
				for(int j=0; j<fingers.count(); j++)
				{
					if(ldata.fpos[i].x < ldata.fpos[j].x)
					{
						tvec = ldata.fpos[i];
						ldata.fpos[i] = ldata.fpos[j];
						ldata.fpos[j] = tvec;

						tvec = ldata.fdir[i];
						ldata.fdir[i] = ldata.fdir[j];
						ldata.fdir[j] = tvec;

						tdist = ldata.dist[i];
						ldata.dist[i] = ldata.dist[j];
						ldata.dist[j] = tdist;
					}
				}
			}

			//avgPos /= (float)fingers.count();
			//       std::cout << "Hand has " << fingers.count()
			//                 << " fingers, average finger tip position" << avgPos << std::endl;
			//std::cout<< avgPos.z<<std::endl;
			
			//if(fingers.count() == 1)
			{
				//pos = avgPos;
// 				pos = fingers[0].tipPosition();
// 				dir = fingers[0].direction();
// 				pos1 = fingers[0].tipPosition();
// 				dir1 = fingers[0].direction();
// 
// 				pos2 = fingers[1].tipPosition();
// 				dir2 = fingers[1].direction();
// 				fcnt = fingers.count();
				
				
			}
			
			
		}


		// Get the hand's sphere radius and palm position
		//     std::cout << "Hand sphere radius: " << hand.sphereRadius()
		//               << " mm, palm position: " << hand.palmPosition() << std::endl;

		// Get the hand's normal vector and direction
		const Vector normal = hand.palmNormal();
		const Vector direction = hand.direction();
		palmpos = hand.palmPosition();
		palmdir = direction;

		ldata.ppos = palmpos;
		ldata.pdir = palmdir;
		// Calculate the hand's pitch, roll, and yaw angles
		//     std::cout << "Hand pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, "
		//               << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, "
		//               << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl;
	}

	// Get gestures


// 	if (!frame.hands().isEmpty() || !gestures.isEmpty()) {
// 		std::cout << std::endl;
// 	}
}

void SampleListener::onFocusGained(const Controller& controller) {
	std::cout << "Focus Gained" << std::endl;
}

void SampleListener::onFocusLost(const Controller& controller) {
	std::cout << "Focus Lost" << std::endl;
}