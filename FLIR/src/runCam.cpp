#include <iostream>
#include <string>
#include <fstream>
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

		// Initialise FLIRCamera instance from the first available camera
        FLIRCamera Fcam (cam_list.GetByIndex(0), config);

        // How many frames to take?
        unsigned long num_frames = config["camera"]["num_frames"].value_or(0);

        // Allocate memory for the image data (given by size of image and buffer size)
        unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

		// Setup and start the camera
        Fcam.InitCamera();

        // Acquire the images, saving them to "image_array", and call "CallbackFunc"
		// after each image is retrieved.
        Fcam.GrabFrames(num_frames, image_array, CallbackFunc);

        // Save the data as a FITS file
        cout << "Saving Data" << endl;
        Fcam.SaveFITS(image_array, Fcam.buffer_size);

		// Turn off camera
        Fcam.DeinitCamera();

		// Free the memory
        free(image_array);

    }

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
