#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "QHYCamera.h"
#include "qhyccd.h"
#include "helperFunc.h"
#include "toml.hpp"

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
int CallbackFunc (unsigned char* data){

    cout << "I could be doing something with the data here..." << endl;

    return 1;
}


int main(int argc, char **argv) {

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    string config_file;

    // Check if config file path is passed as argument
    if (argc > 2) {
        cout << "Too many arguments!" << endl;
        exit(1);
    } else if (argc < 2){
        // Load default config if nothing is passed
        cout << "No CONFIG file loaded" << endl;
        cout << "Will attempt to load default CONFIG" << endl;
        config_file = string("../config/defaultConfig.toml");
    } else {
        // Assign config file value as string
        config_file = string("../config/") + string(argv[1]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    toml::table config = toml::parse_file(config_file);

    // Init SDK
  	unsigned int retVal = InitQHYCCDResource();
  	if (QHYCCD_SUCCESS == retVal){
  		printf("SDK resources initialized.\n");
  	}
  	else{
  		printf("Cannot initialize SDK resources, error: %d\n", retVal);
  	return 1;
  	}

  	// Scan cameras
  	int camCount = ScanQHYCCD();
  	if (camCount > 0){
  		printf("Number of QHYCCD cameras found: %d \n", camCount);
  	}
  	else{
  		printf("No QHYCCD camera found, please check USB or power.\n");
  		return 1;
  	}

  	//Get first camera
  	bool camFound = false;
  	char camId[32];

    retVal = GetQHYCCDId(0, camId);
    if (QHYCCD_SUCCESS == retVal){
    	printf("Application connected to the following camera from the list: cameraID = %s\n", camId);
    	camFound = true;
    }

  	if (!camFound){
    	printf("The detected camera is not QHYCCD or other error.\n");
    	// release sdk resources
    	retVal = ReleaseQHYCCDResource();
    	if (QHYCCD_SUCCESS == retVal){
      		printf("SDK resources released.\n");
    	} else{
      		printf("Cannot release SDK resources, error %d.\n", retVal);
    	}
    	return 1;
  	}
    // Open Camera
    qhyccd_handle *pCamHandle = OpenQHYCCD(camId);
    if (pCamHandle != NULL){
        printf("Open QHYCCD success.\n");
    }else {
        printf("Open QHYCCD failure.\n");
        return 1;
    }

    // Get the settings for the particular camera
    toml::table cam_config = *config.get("QHYcamera")->as_table();

    // Initialise QHYCamera instance from the first available camera
    QHYCamera Qcam (pCamHandle, cam_config);

    // How many frames to take?
    unsigned long num_frames = cam_config["camera"]["num_frames"].value_or(0);

	    // Setup and start the camera
    if(Qcam.InitCamera()){
    	printf("Init failed");
    	return 1;
    }

    // get requested memory length
    unsigned long length = Qcam.imsize*sizeof(unsigned char)*Qcam.pixel_format/8*Qcam.buffer_size;
	unsigned char *arrayData;

    if (length > 0){
        arrayData = (unsigned char*)malloc(length);
        memset(arrayData, 0, length);
        printf("Allocated memory for return array: %lu [uchar].\n", length);
    }else {
        printf("Cannot allocate memory for array.\n");
        return 1;
    }

    // Acquire the images, saving them to "image_array", and call "CallbackFunc"
	    // after each image is retrieved.
    if(Qcam.GrabFrames(num_frames, arrayData, CallbackFunc)){
		printf("Image taking failed");
		return 1;
    }

	// Turn off camera
    Qcam.DeinitCamera();

	// Free the memory
    free(array_data);

    // release sdk resources
    retVal = ReleaseQHYCCDResource();
    if (QHYCCD_SUCCESS == retVal){
      printf("SDK resources released.\n");
    }else {
        printf("Cannot release SDK resources, error %d.\n", retVal);
        return 1;
    }

    return 0;
}
