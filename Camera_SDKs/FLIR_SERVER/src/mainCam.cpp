#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "FLIRCamera.h"
#include "Spinnaker.h"
#include "toml.hpp"

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


	/* NEED TO CHECK FOR STOP MESSAGE, AND RETURN ONE IF THAT IS THE CASE */

	/* ALSO, CHECK IF THERE IS A FEED REQUEST, AND SEND LATEST IMAGE */

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


        int ret_val = 0;
        while (ret_val = 0){

		    // WAIT FOR MESSAGE

		    if message == RECONFIGURE){

		    	Fcam.ReconfigureAll(MESSAGE)

		    }
		    else if message == Single{

		    	int result = 0;

				// How many frames to take?
				unsigned long num_frames = MESSAGE

				Fcam.savefile_dir = MESSAGE

				Fcam.buffer_size = num_frames;

				// Allocate memory for the image data (given by size of image and buffer size)
				unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.imsize*Fcam.buffer_size);

				// Acquire the images, saving them to "image_array", and call "CallbackFunc"
				// after each image is retrieved.
				result = Fcam.GrabFrames(num_frames, image_array, CallbackFunc);

				// Save the data as a FITS file
				cout << "Saving Data" << endl;
				Fcam.SaveFITS(image_array, Fcam.buffer_size);

			   	// Free the memory
		    	free(image_array);



		    }else if message == BURST{

		    	int result = 0;
				// How many frames to take?
				unsigned long num_frames = MESSAGE

				temp_savefile_dir = MESSAGE

				int img_no = 0;

				Fcam.buffer_size = num_frames;

				// Allocate memory for the image data (given by size of image and buffer size)
				unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);
		    	while (result == 0){

		    		string img_no_str = std::to_string(img_no);
		    		Fcam.savefile_dir = temp_savefile_dir + "_" + img_no_str;

					// Acquire the images, saving them to "image_array", and call "CallbackFunc"
					// after each image is retrieved. CallbackFunc to return 1 when exiting!
					result = Fcam.GrabFrames(num_frames, image_array, CallbackFunc);

					// Save the data as a FITS file
					cout << "Saving Data" << endl;

					Fcam.SaveFITS(image_array, Fcam.buffer_size);

					img_no++;

					}

			   	// Free the memory
				free(image_array);


		    }else if message == CONTINUOUS{

		    	int result = 0;
				// How many frames to take?
				unsigned long num_frames = 1000000;

				Fcam.buffer_size = MESSAGE

				// Allocate memory for the image data (given by size of image and buffer size)
				unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);
		    	while (result == 0){


					// Acquire the images, saving them to "image_array", and call "CallbackFunc"
					// after each image is retrieved. CallbackFunc to return 1 when exiting!
					result = Fcam.GrabFrames(num_frames, image_array, CallbackFunc);

					}

			   	// Free the memory
				free(image_array);


		    }else if message == DISCONECT{

				ret_val = 1;
				// Turn off camera
				Fcam.DeinitCamera();
			}
		}
	}

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
