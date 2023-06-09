#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "FLIRCamera.h"
#include "Spinnaker.h"
#include "toml.hpp"
#include <pthread.h>
#include "runFLIRCam.h"
#include "globals.h"

using namespace Spinnaker;
using namespace std;

/* Program to run the camera based on a configuration file
   given to it in "toml" format. It will take a certain number
   of frames, during which it will apply a function "CallbackFunc"
   on each image as they are acquired. It then saves all of the image
   data into a FITS file (the specifications of which are in the
   configuration file.
*/


/* A function that can be used to perform real time data analysis on a frame (eg Fringe Tracking)
   INPUTS:
      data - raw image data of a single frame
   Returns 0 on regular exit, 1 if the stopping flag is set
*/
int CallbackFunc (unsigned short* data){

	if (GLOB_STOPPING==1){
		
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	GLOB_STOPPING = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		
		return 1;
	
	}

    if (GLOB_CALLBACK(data)){
        cerr << "Callback function error. Stopping Camera" << endl;
        pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	GLOB_STOPPING = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		return 1;
        }

    return 0;
}


/* Function to reconfigure all parameters
    INPUTS:
       c - configuration (json) structure with all the values
       Fcam - FLIRCamera class 
*/
void reconfigure(configuration c, FLIRCamera& Fcam){

	Fcam.ReconfigureAll(c.gain, c.exptime, c.width, c.height, c.offsetX, c.offsetY, c.blacklevel, c.buffersize, c.savedir);
	
	return;
}


/* Main pThread function to run the camera with a separate thread */ 
void *runCam(void*) {
	
	// Cam status of 1 indicates camera is starting up
	pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CAM_STATUS = 1;
    string config_file = GLOB_CONFIGFILE;
	pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	
	cout << GLOB_CONFIGFILE << endl;
	
    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;


    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	GLOB_CAM_STATUS = 0;
    	string config_file = GLOB_CONFIGFILE;
        pthread_exit(NULL);
    }

    // Parse the configuration file
    
	//toml::table config;
    toml::table config = toml::parse_file(config_file);


    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnaker_library_version = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnaker_library_version.major << "." << spinnaker_library_version.minor
         << "." << spinnaker_library_version.type << "." << spinnaker_library_version.build << endl
         << endl;

    // Retrieve list of cameras from the system
    CameraList cam_list = system->GetCameras();

    // Are cameras connected?
    if(cam_list.GetSize() == 0) {
        cerr << "No camera connected" << endl;
        pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	GLOB_CAM_STATUS = 0;
    	string config_file = GLOB_CONFIGFILE;
        pthread_exit(NULL);
    }else {
        // Get the settings for the particular camera
        toml::table cam_config = *config.get("FLIRcamera")->as_table();
        string serialNum = cam_config["cam_ID"].value_or("00000000");
        int sleeptime = cam_config["sleep_time"].value_or(1000000);


	    // Initialise FLIRCamera instance from the serial number
        FLIRCamera Fcam (cam_list.GetBySerial(serialNum), cam_config);
        
	    // Setup and start the camera
        Fcam.InitCamera();
        
        // Cam status of 2 indicates camera is waiting   
        pthread_mutex_lock(&GLOB_FLAG_LOCK);
        GLOB_CAM_STATUS = 2;
        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
        

        // Run a waiting loop as long as the camera remains "waiting"
        while (GLOB_CAM_STATUS==2){
        
            // Check if camera needs reconfiguring
        	if(GLOB_RECONFIGURE==1){
        		reconfigure(GLOB_CONFIG_PARAMS,Fcam);
        		
        		pthread_mutex_lock(&GLOB_FLAG_LOCK);
        		GLOB_RECONFIGURE=0;
        		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
        	}
        
            // Check if camera needs to start acquisition
        	if(GLOB_RUNNING==1){

				int finish = 0;
				
				// How many frames to take?
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				unsigned long num_frames = GLOB_NUMFRAMES;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				
				int SAVE_FLAG = 1;
				
				// No saving if num_frames = 0; continuous acquisition
				if(num_frames == 0){
					SAVE_FLAG = 0;
					num_frames = 100000;
				};

				unsigned long buffer_no = 0;
				unsigned long save_no = 0;

				// Allocate memory for the image data (given by size of image and buffer size), and mutex array
				GLOB_IMG_ARRAY = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.imsize*Fcam.buffer_size);
				GLOB_IMG_MUTEX_ARRAY = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*Fcam.buffer_size);
				for(int i=0; i<Fcam.buffer_size; i++){
					GLOB_IMG_MUTEX_ARRAY[i] = PTHREAD_MUTEX_INITIALIZER;
				}
				
				while (finish == 0){
				
			        // Acquire the images, saving them to GLOB_IMG_ARRAY, and call "CallbackFunc"
			        // after each image is retrieved. CallbackFunc to return 1 when exiting!
					finish = Fcam.GrabFrames(num_frames, buffer_no, CallbackFunc);

					if (SAVE_FLAG == 1){
						// Save the data as a FITS file
						cout << "Saving Data" << endl;

						string save_no_str = std::to_string(save_no);
						Fcam.savefilename = Fcam.savefilename_prefix + "_" + save_no_str;
						
						Fcam.SaveFITS(num_frames, buffer_no);
						
						save_no++;
					}
					
					// Index of the next available spot in the circular buffer
					buffer_no = (buffer_no+num_frames)%Fcam.buffer_size;
				}
								
			   	// Free the memory
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
        		GLOB_RUNNING = 0;
        		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
        		
        		free(GLOB_IMG_ARRAY);
				free(GLOB_IMG_MUTEX_ARRAY);
			
				// Reset latest file for plate solver to stop solving when not running
				pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
    			GLOB_LATEST_FILE = "CAMERA_NOT_SAVING";
   				pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);

			}
			
			usleep(sleeptime); // Sleep to save resources

		}
		
    Fcam.DeinitCamera(); // Deinit camera
	}
	
    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();
    

    pthread_exit(NULL);
}
