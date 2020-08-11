#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm>
#include <string>
#include "modules/acquisition.h"
#include "modules/saveFITS.h"
#include "modules/realTimeFunc.h"
#include "Spinnaker.h"
#include "cpptoml/cpptoml.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

// !!! NOT ACTUALLY SET UP YET (IS THE SAME AS RUNCAM TO TEST COMPILATION) // 
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
        config_file = string(argv[1]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    std::shared_ptr<cpptoml::table> config = cpptoml::parse_file(config_file);

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
        // Read some data from the config file
        int width = config->get_qualified_as<int>("camera.width").value_or(0);
        int height = config->get_qualified_as<int>("camera.height").value_or(0);
        long buffer_size = config->get_qualified_as<long>("fits.buffer_size").value_or(0);
        unsigned long num_frames = config->get_qualified_as<unsigned long>("camera.num_frames").value_or(0);

        // Allocate memory for the image data
        vector<int> fits_array (width*height*buffer_size);
        Times times_struct;

        // Grab frames from the first available camera
        // Use the function "RealTimeFunc" on each separate frame in real time
        GrabFrames(cam_list.GetByIndex(0), config, num_frames, fits_array, times_struct, 1, RealTimeFunc);

        // Save the data as a FITS file
        cout << "Saving Data" << endl;
        SaveFITS(config, fits_array, times_struct);
    }

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
