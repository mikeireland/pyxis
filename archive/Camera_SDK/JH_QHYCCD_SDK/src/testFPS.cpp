#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "QHYCamera.h"
#include "qhyccd.h"
#include "toml.hpp"

using namespace std;

/* Program to test the FPS response of a camera.
   Essentially runs the camera at different exposure times
   and ROI dimensions and saves the resultant FPS into a csv file.
   ARGUMENT: name of config file (in config/) to apply
*/


/* Struct to hold the data for each line of the CSV */
struct CsvLine {

    // Single frame exposure time
    int exp_time;

    // Square dimensions of image
    int dimensions;

    // Number of frames to take
    unsigned long num_frames;

    // Exposure time over all frames
    double total_exp_time;

    // Resultant FPS of camera
    double FPS;
};


/* Function to run a test of the FPS for a single exposure time and ROI.
   INPUTS:
      pCam - Spinnaker camera pointer
      config - TOML configuration table
      line_data - CsvLine struct to hold the data for a line in the output CSV
*/
void Test(char* camId, toml::table config, CsvLine& line_data) {

    // Reassign exposure time and ROI dimensions
    toml::table* cam_table = config.get("camera")->as_table();
    cam_table->insert_or_assign("exposure_time",line_data.exp_time);
    cam_table->insert_or_assign("width",line_data.dimensions);
    cam_table->insert_or_assign("height",line_data.dimensions);

	cout << "Starting init" << endl << endl;

	// Open Camera
    qhyccd_handle *pCam = OpenQHYCCD(camId);
    if (pCam != NULL){
        printf("Open QHYCCD success.\n");
    }else {
        printf("Open QHYCCD failure.\n");
    }

    // Get a FLIRCamera instance
    QHYCamera Qcam (pCam, config);

    // Allocate memory for the image data
    unsigned char *image_array = (unsigned char*)malloc(Qcam.width*Qcam.height*sizeof(unsigned char)*Qcam.pixel_format/8*Qcam.buffer_size);

    // Init camera
    Qcam.InitCamera();

    // Acquire the images, saving them to "image_array". No callback function
    Qcam.GrabFrames(line_data.num_frames, image_array, NULL);
    
    // Free memory (we don't care about the actual image data)
    free(image_array);

    cout << "Saving Data" << endl << endl;

    // Calculate FPS
    line_data.total_exp_time = Qcam.total_exposure;
    line_data.FPS = static_cast<double>(line_data.num_frames)*1000/line_data.total_exp_time;

    // Deinit camera
    Qcam.DeinitCamera();
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
        config_file = string(argv[1]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    toml::table config = toml::parse_file(config_file);

    SDKVersion();

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


    // Get the settings for the particular camera
    toml::table cam_config = *config.get("QHYcamera")->as_table();

    // Filename for output file csv
    std::ofstream out_file("../data/FPS_Test.csv");

    // List of exposure times to check
    int exp_times[15] = {100000, 50000, 30000, 20000, 15000, 10000, 8000, 7000, 6000, 5000, 4000, 2000, 1500, 1000, 500};

    // List of ROI dimensions to check
    int dimensions[12] = {1000, 900, 800, 700, 600, 500, 400, 300, 200, 100, 64, 32};

    out_file << " Frame Exposure Time (us), Dimensions (px), Total Exposure Time (ms), Number of Frames, FPS \n";

    CsvLine line_data;

    line_data.num_frames = cam_config["camera"]["num_frames"].value_or(0);

    // Loop over all ROI dimensions and exposure times
    for (int i=0;i<12;i++){

        line_data.dimensions = dimensions[i];

        for (int j=0;j<15;j++){

            cout << "Running number " << i << "," << j << endl;

            line_data.exp_time = exp_times[j];

            // Run camera
            Test(camId,cam_config,line_data);

            char buffer [50];

            // Save as an output string
            sprintf (buffer, " %d, %d, %d, %lu, %f \n", line_data.exp_time, line_data.dimensions,
                     static_cast<int>(line_data.total_exp_time), line_data.num_frames, line_data.FPS);

            // Save string to file
            out_file << buffer;
        }
    }

    out_file.close();


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
