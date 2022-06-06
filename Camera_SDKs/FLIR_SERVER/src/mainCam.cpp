#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "FLIRCamera.h"
#include "Spinnaker.h"
#include "toml.hpp"
#include <pthread.h>
#include "mainCam.h"
#include "helperFunc.h"

using namespace Spinnaker;
using namespace std;




/* Program to run the camera based on a configuration file
   given to it in "toml" format. It will take a certain number
   of frames, during which it will apply a function "CallbackFunc"
   on each image as they are acquired. It then saves all of the image
   data into a FITS file (the specifications of which are in the
   configuration file.

   ARGUMENT: name of config file (in config/) to apply
*/


/* A function that can be used to perform real time data analysis on a frame (eg Fringe Tracking)
   INPUTS:
      data - raw image data of a single frame
*/
int CallbackFunc (unsigned short* data){

	if (GLOB_STOPPING==1){
		
		pthread_mutex_lock(&flag_lock);
    	GLOB_STOPPING = 0;
		pthread_mutex_unlock(&flag_lock);
		
		return 0;
	
	}

    cout << "I could be doing something with the data here..." << endl;

    return 1;
}



int reconfigure(configuration c, FLIRCamera Fcam){

	int ret_val = 0;

	Fcam.ReconfigureAll(c.gain, c.exptime, c.width, c.height, c.offsetX, c.offsetY, c.blacklevel, c.buffersize, c.savedir);
	
	return ret_val;
}



//void main(void*) {
int main(int argc, char **argv) {
	
	pthread_mutex_lock(&flag_lock);
    GLOB_CAM_STATUS = 1;
    string config_file = GLOB_CONFIGFILE;
	pthread_mutex_unlock(&flag_lock);
	
	cout << GLOB_CONFIGFILE << endl;
	
    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;


    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        pthread_exit(NULL);
    }

	cout << "about to connect" << endl;
    // Parse the configuration file
    
	//toml::table config;
    toml::table config = toml::parse_file(config_file);
    
    cout << "about to connect" << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnaker_library_version = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnaker_library_version.major << "." << spinnaker_library_version.minor
         << "." << spinnaker_library_version.type << "." << spinnaker_library_version.build << endl
         << endl;

    // Retrieve list of cameras from the system
    CameraList cam_list = system->GetCameras();

    if(cam_list.GetSize() == 0) {
        cerr << "No camera connected" << endl;
        pthread_exit(NULL);
    }else {
        // Get the settings for the particular camera
        toml::table cam_config = *config.get("testFLIRcamera")->as_table();


	// Initialise FLIRCamera instance from the first available camera
        FLIRCamera Fcam (cam_list.GetByIndex(0), cam_config);

	 // Setup and start the camera
        Fcam.InitCamera();
        
        pthread_mutex_lock(&flag_lock);
        GLOB_CAM_STATUS = 2;
        pthread_mutex_unlock(&flag_lock);

		

        while (GLOB_CAM_STATUS==2){
        
        	if(GLOB_RECONFIGURE==1){
        		configuration new_params;
        		reconfigure(new_params,Fcam);
        		
        		pthread_mutex_lock(&flag_lock);
        		GLOB_RECONFIGURE=0;
        		pthread_mutex_unlock(&flag_lock);
        	}
        
        	if(GLOB_RUNNING==1){

				int finish = 0;
				// How many frames to take?
				
				pthread_mutex_lock(&flag_lock);
				unsigned long num_frames = GLOB_NUMFRAMES;
				pthread_mutex_unlock(&flag_lock);
				
				int SAVE_FLAG = 1;
				
				if(num_frames == 0){
					SAVE_FLAG = 0;
					num_frames = 100000;
				};

				unsigned long buffer_no = 0;
				unsigned long save_no = 0;

				// Allocate memory for the image data (given by size of image and buffer size)
				unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.imsize*Fcam.buffer_size);
				while (finish == 0){
				
					// Acquire the images, saving them to "image_array", and call "CallbackFunc"
					// after each image is retrieved. CallbackFunc to return 1 when exiting!
					finish = Fcam.GrabFrames(num_frames, buffer_no, image_array, CallbackFunc);

					if (SAVE_FLAG == 1){
						// Save the data as a FITS file
						cout << "Saving Data" << endl;

						string save_no_str = std::to_string(save_no);
						Fcam.savefilename = Fcam.savefilename_prefix + "_" + save_no_str;
						
						Fcam.SaveFITS(image_array, num_frames, buffer_no);
						
						save_no++;
					}
					buffer_no = (buffer_no+num_frames)%Fcam.buffer_size;
				}
								
			   	// Free the memory
				free(image_array);
				
				pthread_mutex_lock(&flag_lock);
        		GLOB_RUNNING = 0;
        		pthread_mutex_unlock(&flag_lock);
			}
			
			//Sleep for a bit??

		}
	}

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();
    

    pthread_exit(NULL);
}
