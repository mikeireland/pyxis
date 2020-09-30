#include <iostream>
#include <string>
#include <fstream>
#include "FLIRCamera.h"
#include "Spinnaker.h"
#include "toml.hpp"
#include "AC_tracking.h"

using namespace Spinnaker;
using namespace std;

/* Program to test fringe locking on the camera based on a configuration file
   given to it in "toml" format.

   ARGUMENT: name of config file (in config/) to apply
*/

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

        // Allocate memory for the image data (given by size of image and buffer size)
        unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

        // Get the settings for the fringes
        toml::table fringe_config = *config.get("fringe")->as_table();

        // Set up AC delays
        CalcTrialFringes(fringe_config);

        Fcam.GrabFrames(10, image_array, findDelay);

    }

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
