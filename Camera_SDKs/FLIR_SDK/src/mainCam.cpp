#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "FLIRCamera.h"
#include "Spinnaker.h"
#include "toml.hpp"

using namespace Spinnaker;
using namespace std;

/* CURRENT FLAGS:
	Stop exp = END_EXP==1
	Start exp = EXPOSING==1
	Camera status = CAM_STATUS
	NUM_FRAMES (essentially; how many to save?)
	reconfigure = RECONFIGURE
*/




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

	if END_EXP==1{
	
		return 0
	
	}
	/* NEED TO CHECK FOR STOP MESSAGE, AND RETURN ZERO IF THAT IS THE CASE */
	
	/* ALSO, CHECK IF THERE IS A FEED REQUEST, AND SEND LATEST IMAGE */

    cout << "I could be doing something with the data here..." << endl;

    return 1;
}

struct configuration{
    int gain; 
    int exptime; 
    int width; 
    int height; 
    int offsetX; 
    int offsetY; 
    int blacklevel;
    int buffersize;
    string savedir;
};


int reconfigure(configuration c, FLIRCamera Fcam){

	int ret_val = 0;

	Fcam.ReconfigureAll(c.gain, c.exptime, c.width, c.height, c.offsetX, c.offsetY, c.blacklevel, c.buffersize, c.savedir);
	
	return ret_val;
}

int main(int argc, char **argv) {

    int CAM_STATUS = 0;

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
        return -1;
    }
    else {
        // Get the settings for the particular camera
        toml::table cam_config = *config.get("testFLIRcamera")->as_table();

	

	// Initialise FLIRCamera instance from the first available camera
        FLIRCamera Fcam (cam_list.GetByIndex(0), cam_config);

	 // Setup and start the camera
        Fcam.InitCamera();
        
        int CAM_STATUS = 1;
        while (CAM_STATUS==1){
        
        	if(RECONFIGURE==1){
        		new_params = ????;
        		reconfigure(new_params,Fcam);
        		RECONFIGURE=0;
        	}
        
        	if(EXPOSING==1){

				int finish = 0;
				// How many frames to take?
				unsigned long num_frames = NUM_FRAMES
				
				SAVE_FLAG = 1
				
				if(num_frames == 0){
					SAVE_FLAG = 0
					num_frames = 100000
				}

				int img_no = 0;

				// Allocate memory for the image data (given by size of image and buffer size)
				unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);
				while (finish == 0){
				
					// Acquire the images, saving them to "image_array", and call "CallbackFunc"
					// after each image is retrieved. CallbackFunc to return 1 when exiting!
					finish = Fcam.GrabFrames(num_frames, image_array, CallbackFunc);

					if (SAVE_FLAG == 1){
						// Save the data as a FITS file
						cout << "Saving Data" << endl;

						string img_no_str = std::to_string(img_no);
						Fcam.savefilename = Fcam.savefilename_prefix + "_" + img_no_str;
						
						Fcam.SaveFITS(image_array, Fcam.buffer_size);
						
						img_no++;
					}
				}
								
			   	// Free the memory
				free(image_array);
				EXPOSING = 0
			}
			
			//Sleep for a bit??

		}
	}

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
