#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "globals.h"
#include "mainCam.h"
#include <commander/commander.h>

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
		ret_msg = "Camera Running! Saving " + std::to_string(GLOB_NUMFRAMES) + " frames per file";
	}else if(GLOB_STOPPING == 1){
		ret_msg = "Camera Stopping";
	}else{
		ret_msg = "Camera Waiting";
	}
	
	return ret_msg;
	
}

//string msg_reconfigure(configuration c){

//}

string msg_connectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		printf("gday");
		pthread_create(&GLOB_CAMTHREAD, NULL, runCam, NULL);

		ret_msg = "Connecting Camera";
	}else{
		ret_msg = "Camera Already Connecting/Connected!";
	}	
	
	return ret_msg;
}

string msg_disconnectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_CAM_STATUS = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		
		pthread_join(GLOB_CAMTHREAD, NULL);
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
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				GLOB_NUMFRAMES = num_frames;
				GLOB_RUNNING = 1;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
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
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				GLOB_STOPPING = 1;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
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

string msg_getlatestfilename(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
				ret_msg = GLOB_LATEST_FILE;
				pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);
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

//struct image_return{}

string msg_getlatestimage(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_LATEST_IMG_INDEX_LOCK);
				int img_index = GLOB_LATEST_IMG_INDEX;
				pthread_mutex_unlock(&GLOB_LATEST_IMG_INDEX_LOCK);
				
				unsigned short *ret_image_array;
				ret_image_array = (unsigned short*)malloc(sizeof(unsigned short)*GLOB_IMSIZE);
				
				pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[img_index]);
				memcpy(ret_image_array,GLOB_IMG_ARRAY+GLOB_IMSIZE*img_index,GLOB_IMSIZE*2);
				pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[img_index]);
				
				// COMPRESSION???
				
				
				printf( "%hu\n", ret_image_array[0] );
				
				ret_msg = "Retrieved Image Successfully";
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



int main(int argc, char **argv) {

	int port = 2543;
	msg_connectcam();
	string msg;
	
	int i = 0;
	while(i<5){
		msg = msg_getstatus();
		cout << msg << endl;
		msg = msg_startcam(5);
		cout << msg << endl;
		sleep(1);
		msg = msg_getlatestfilename();
		cout << msg << endl;
		msg = msg_getlatestimage();
		cout << msg << endl;
		i++;
	}
	i = 0;
	while(i<3){
		msg = msg_getstatus();
		cout << msg << endl;
		msg = msg_stopcam();
		cout << msg << endl;
		sleep(1);
		i++;
	}
	msg = msg_disconnectcam();
	cout << msg << endl;
	sleep(1);

}
