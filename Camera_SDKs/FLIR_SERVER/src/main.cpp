#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "helperFunc.h"
#include "mainCam.h"

using namespace std;


string msg_getstatus(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		ret_msg = "Camera Not Connected!";
	}else if(GLOB_CAM_STATUS == 1){
		ret_msg = "Camera Connecting";
	}else if(GLOB_RECONFIGURE == 1){
		ret_msg = "Camera Reconfiguring";
	}else if(GLOB_RUNNING == 1){
		ret_msg = "Camera Running! Saving " + std::to_string(GLOB_NUMFRAMES) + "per file";
	}else if(GLOB_STOPPING == 1){
		ret_msg = "Camera Stopping";
	}else{
		ret_msg = "Camera Waiting";
	};
	
	return ret_msg;
	
}

//string msg_reconfigure(configuration c){

//}

string msg_connectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){

		pthread_create(&camthread, NULL, runCam, NULL);

		ret_msg = "Connecting Camera";
	}else{
		ret_msg = "Camera Already Connecting/Connected!";
	}	
	
	return ret_msg;
}

string msg_disconnectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		pthread_mutex_lock(&flag_lock);
		GLOB_CAM_STATUS = 0;
		pthread_mutex_unlock(&flag_lock);
		
		pthread_join(camthread, NULL);
		ret_msg = "Disconnected Camera";
		
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}
	
	return ret_msg;
}

string msg_startcam(int num_frames){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 0){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&flag_lock);
				GLOB_NUMFRAMES = num_frames;
				GLOB_RUNNING = 1;
				pthread_mutex_unlock(&flag_lock);
				ret_msg = "Starting Camera Exposures";
			}else{
				ret_msg = "Camera Busy!";
			}
		}else{
			ret_msg = "Camera already running!";
		}
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}
	
	return ret_msg;

}

string msg_stopcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&flag_lock);
				GLOB_STOPPING = 1;
				pthread_mutex_unlock(&flag_lock);
				ret_msg = "Stopping Camera Exposures";
			}else{
				ret_msg = "Camera Busy!";
			}
		}else{
			ret_msg = "Camera not running!";
		}
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}
	
	return ret_msg;
}

//??? get_image(){

//}

int main(int argc, char **argv) {

	int port = 2543;
	msg_connectcam();
	

}
