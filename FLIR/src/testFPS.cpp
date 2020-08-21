#include <iostream>
#include <string>
#include <fstream>
#include "modules/acquisition.h"
#include "modules/realTimeFunc.h"
#include "Spinnaker.h"
#include "cpptoml/cpptoml.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

struct CsvLine {
    int exp_time;
    int dimensions;
    double total_exp_time;
    unsigned long num_frames;
    double FPS;
};

void ReplaceCameraConfigInt(std::shared_ptr<cpptoml::table> config, std::string key, int value){

    std::shared_ptr<cpptoml::table> camera_table = config->get_table("camera");
    camera_table->erase(key);
    camera_table->insert(key,value);

}

void Test(Spinnaker::CameraPtr pCam, std::shared_ptr<cpptoml::table> config, CsvLine& line_data) {

    ReplaceCameraConfigInt(config,"exposure_time",line_data.exp_time);
    ReplaceCameraConfigInt(config,"height",line_data.dimensions);
    ReplaceCameraConfigInt(config,"width",line_data.dimensions);

    int buffer_size = config->get_qualified_as<int>("fits.buffer_size").value_or(0);


    cout << "allocating array" << endl;
    // Allocate memory for the image data
    unsigned short *fits_array = (unsigned short*)malloc(sizeof(unsigned short) * line_data.dimensions*line_data.dimensions*buffer_size);
    Times times_struct;
    cout << "allocated array" << endl;
    // Grab frames from the first available camera
    // Use the function "RealTimeFunc" on each separate frame in real time
    GrabFrames(pCam, config, line_data.num_frames, fits_array, times_struct, 0, RealTimeFunc);

    free(fits_array);

    // Save the data as a FITS file
    cout << "Saving Data" << endl << endl;

    line_data.total_exp_time = times_struct.total_exposure;
    line_data.FPS = static_cast<double>(line_data.num_frames)*1000/times_struct.total_exposure;

}


/* Program to run the camera based on a configuration file
   given to it in "toml" format. It will take a certain number
   of frames, during which it will apply a function "RealTimeFunc"
   on each image as they are acquired. It then saves all of the image
   data into a FITS file (the specifications of which are in the
   configuration file.

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

        std::ofstream out_file("../data/FPS_Test.csv");
        int exp_times[15] = {100000, 50000, 30000, 20000, 15000, 10000, 8000, 7000, 6000, 5000, 4000, 2000, 1500, 1000, 500};
        int dimensions[12] = {1000, 900, 800, 700, 600, 500, 400, 300, 200, 100, 64, 32};

        out_file << " Frame Exposure Time (us), Dimensions (px), Total Exposure Time (ms), Number of Frames, FPS \n";

        CsvLine line_data;

        line_data.num_frames = config->get_qualified_as<unsigned long>("camera.num_frames").value_or(0);

        for (int i=0;i<12;i++){

            line_data.dimensions = dimensions[i];

            for (int j=0;j<15;j++){

                cout << "Running number " << i << "," << j << endl;

                line_data.exp_time = exp_times[j];

                Test(cam_list.GetByIndex(0),config,line_data);

                char buffer [50];

                sprintf (buffer, " %d, %d, %d, %lu, %f \n", line_data.exp_time, line_data.dimensions, static_cast<int>(line_data.total_exp_time), line_data.num_frames, line_data.FPS);

                out_file << buffer;
            }
        }

        out_file.close();
    }

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    return 0;
}
