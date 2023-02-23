#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "QHYCamera.h"
#include "qhyccd.h"
#include "toml.hpp"
#include <pthread.h>
#include "runQHYCam.h"
#include "globals.h"

using namespace std;

/* Program to run the camera based on a configuration file
   given to it in "toml" format. It will take a certain number
   of frames, during which it will apply a function "CallbackFunc"
   on each image as they are acquired. It then saves periodically a number 
   of the image data into a FITS file (the specifications of which are 
   in the configuration file.
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
       Qcam - QHYCamera class 
    Returns 0 on success, 1 on error
*/
int reconfigure(configuration c, QHYCamera& Qcam){

	int ret_val = 0;

	ret_val = Qcam.ReconfigureAll(c.gain, c.exptime, c.width, c.height, c.offsetX, c.offsetY, static_cast<int>(c.blacklevel), c.buffersize, c.savedir);
	
	return ret_val;
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

	SDKVersion();

    // Init SDK
  	unsigned int retVal = InitQHYCCDResource();
  	if (QHYCCD_SUCCESS == retVal){
  		printf("SDK resources initialized.\n");
  	}
  	else{
  		printf("Cannot initialize SDK resources, error: %d\n", retVal);
  	    pthread_exit(NULL);
  	}

  	// Scan cameras
  	int camCount = ScanQHYCCD();
  	if (camCount > 0){
  		printf("Number of QHYCCD cameras found: %d \n", camCount);
  	}
  	else{
  		printf("No QHYCCD camera found, please check USB or power.\n");
  		pthread_exit(NULL);
  	}

  	//Get first camera
  	bool camFound = false;
  	char camId[32];

    retVal = GetQHYCCDId(0, camId);
    if (QHYCCD_SUCCESS == retVal){
    	printf("Application connected to the following camera from the list: cameraID = %s\n", camId);
    	camFound = true;
    }

    // Return if no camera is found
  	if (!camFound){
    	printf("The detected camera is not QHYCCD or other error.\n");
    	// release sdk resources
    	retVal = ReleaseQHYCCDResource();
    	if (QHYCCD_SUCCESS == retVal){
      		printf("SDK resources released.\n");
    	} else{
      		printf("Cannot release SDK resources, error %d.\n", retVal);
    	}
    	pthread_exit(NULL);
  	}
    // Open Camera
    qhyccd_handle *pCamHandle = OpenQHYCCD(camId);
    if (pCamHandle != NULL){
        printf("Open QHYCCD success.\n");
    }else {
        printf("Open QHYCCD failure.\n");
        pthread_exit(NULL);
    }

    // Get the settings for the particular camera
    toml::table cam_config = *config.get("QHYcamera")->as_table();


    // Initialise QHYCamera instance from the first available camera
    QHYCamera Qcam (pCamHandle, cam_config);

	 // Setup and start the camera
    if(Qcam.InitCamera()){
    	printf("Init failed");
    	pthread_exit(NULL);
    }

  	// Cam status of 2 indicates camera is waiting     
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CAM_STATUS = 2;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

    // Run a waiting loop as long as the camera remains "waiting"
    while (GLOB_CAM_STATUS==2){
        
        // Check if camera needs reconfiguring
    	if(GLOB_RECONFIGURE==1){

    		if (reconfigure(GLOB_CONFIG_PARAMS,Qcam)){
    		    printf("Reconfig failed");
    		}
    		
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
		    GLOB_IMG_ARRAY = (unsigned short*)malloc(sizeof(unsigned short)*Qcam.imsize*Qcam.buffer_size);
		    GLOB_IMG_MUTEX_ARRAY = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*Qcam.buffer_size);
		    for(int i=0; i<Qcam.buffer_size; i++){
			    GLOB_IMG_MUTEX_ARRAY[i] = PTHREAD_MUTEX_INITIALIZER;
		    }
		    
		    while (finish == 0){
		    
			    // Acquire the images, saving them to GLOB_IMG_ARRAY, and call "CallbackFunc"
			    // after each image is retrieved. CallbackFunc to return 1 when exiting!
			    finish = Qcam.GrabFrames(num_frames, buffer_no, CallbackFunc);
			    
			    // Check if grabFrames returned an error
			    if (finish == 2){
			        printf("Error in Grab Frames");
    	            break;
			    }

			    if (SAVE_FLAG == 1){
				    // Save the data as a FITS file
				    cout << "Saving Data" << endl;

				    string save_no_str = std::to_string(save_no);
				    Qcam.savefilename = Qcam.savefilename_prefix + "_" + save_no_str;
				    
				    Qcam.SaveFITS(num_frames, buffer_no);
				    
				    save_no++;
			    }
			    
			    // Index of the next available spot in the circular buffer
			    buffer_no = (buffer_no+num_frames)%Qcam.buffer_size;
		    }
						    
	       	// Free the memory
		    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    		GLOB_RUNNING = 0;
    		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    		
    		free(GLOB_IMG_ARRAY);
		    free(GLOB_IMG_MUTEX_ARRAY);
	    }
	    
	    sleep(1); // Sleep to save resources

	}

    Qcam.DeinitCamera(); // Deinit camera

    // release SDK resources
    retVal = ReleaseQHYCCDResource();
    if (QHYCCD_SUCCESS == retVal){
      printf("SDK resources released.\n");
    }else {
        printf("Cannot release SDK resources, error %d.\n", retVal);
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

