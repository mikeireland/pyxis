#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm>
#include "../modules/acquisition.h"
#include "../modules/saveFITS.h"
#include "../modules/realTimeFunc.h"
#include "Spinnaker.h"
#include "../../lib/cpptoml/cpptoml.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

int numFrames = 2000;
string filename = "myfunfits.fits";

int main(int argc, char **argv) {

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Check if config file path is passed as argument
    if (argc > 2) {
        cout << "Too many arguments!" << endl;
        exit(1);
    } else if (argc < 2){
        cout << "No CONFIG file loaded" << endl;
        cout << "Will attempt to load default CONFIG" << endl;
        string config_file = string("../config/defaultConfig.toml");
    } else {
        // Assign config file value as string
        string config_file = string(argv[1]);
    }

    // Check whether config file is readable
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Initialize configuration
    auto config = cpptoml::parse_file(config_file);

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    if(camList.GetSize() == 0) {
        cerr << "No camera connected" << endl;
        return -1;
    }
    else {
        int width = config->get_qualified_as<int>("camera.width").value_or(0);
        int height = config->get_qualified_as<int>("camera.height").value_or(0);
        long bufferSize = config->get_qualified_as<long>("fits.bufferSize").value_or(0);

        vector<int> fitsArray (width*height*bufferSize);
        times timesStruct;

        // Run tests on first available camera
        grabFrames(camList.GetByIndex(0), config, numFrames, fitsArray, timesStruct, 0, realTimeFunc);

        saveFITS(filename, config, fitsArray, timesStruct);
    }

    // Clear camera list before releasing system
    camList.Clear();
    // Release system
    system->ReleaseInstance();

    return 0
}
