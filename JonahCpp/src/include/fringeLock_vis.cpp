#include <vector>
#include <iostream>
#include <complex>
#include <fftw3.h>
#include <cmath>
#include "FLIRCamera.h"
#include "fringeLock_vis.h"
#include "ZaberActuator.h"
#include "helperFunc.h"
#include <zaber/motion/binary.h>

using namespace std;
using namespace zaber::motion;

/* FUNCTIONS TO PERFORM FRINGE LOCKING WITH
   NOTE: THERE IS SOME VERSION OF A RACE CONDITION IN HERE. NEED TO CHECK IF
   THIS IS A CONCERN...
*/

static double lock_vis;
static std::vector<double> vis_arr;
static int num_channels, outputA_idx, outputB_idx;

/* High level function to perform fringe locking with visibilities
   INPUTS:
      Fcam - FLIRCamera class
      stage - ZaberActuator class
      fringe_config - table of configuration values for the fringe configuration
*/
void FringeLockVis(FLIRCamera Fcam, ZaberActuator stage, toml::table fringe_config){

    // Keep old exposure time to give back later
    int old_exposure = Fcam.exposure_time;

    // Read from config file
    // How fast to move the actuator during the scan in um/s
    double scan_rate = fringe_config["locking"]["scan_rate"].value_or(0.0);
    // SNR to trigger fringes being "found"
    lock_vis = fringe_config["locking"]["lock_vis"].value_or(0.0);
    // Over what distance should we scan?
    double scan_width = fringe_config["locking"]["scan_width"].value_or(0.0);
    // Exposure time of camera during scan
    Fcam.exposure_time = fringe_config["locking"]["exposure_time"].value_or(0);

    // Number of meters per frame the stage will scan
    double meters_per_frame = scan_rate*(Fcam.exposure_time/1e6);

    // Maximum number of frames to take over the scan
    int num_frames = scan_width/meters_per_frame;

    num_channels = fringe_config["positions"]["num_channels"].value_or(0);
    outputA_idx = fringe_config["positions"]["A"]["indices"][0].value_or(0);
    outputB_idx = fringe_config["positions"]["B"]["indices"][0].value_or(0);

    // Actuator Starting position (wait for max 100s until arrived)
    double start_pos = fringe_config["locking"]["start_pos"].value_or(0.0);
    stage.MoveAbsolute(start_pos, Units::LENGTH_MILLIMETRES, 100);

    // Allocate memory for the image data (given by size of image and buffer size)
    unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

    // Init camera
    Fcam.InitCamera();

    // Start actuator movement
    stage.MoveAtVelocity(scan_rate, Units::VELOCITY_METRES_PER_SECOND);

    // Start frame capture and scan
    Fcam.GrabFrames(num_frames, image_array, VisCalc);

    // Stop actuator movement and retrieve position
    double position = stage.Stop(Units::LENGTH_MILLIMETRES);

    // Deinit camera
    Fcam.DeinitCamera();

    // Return old exposure time
    Fcam.exposure_time = old_exposure;

    // Retrieve Delay and print
    cout << endl << "END LOCKING at: " << position << " millimeters"<< endl;

    // Memory Management and reset
    free(image_array);
}

/* Callback function to be called each time a new frame is taken by the camera during
   fringe locking. Calls FringeLock on a number of pixels, plots the data and checks
   if the SNR is good enough to claim we have found fringes.

   INPUTS:
      frame - Image array data

   OUTPUTS:
      0 if fringes have been found
      1 otherwise
*/
int VisCalc(unsigned short * frame){

    double outputA_sum = 0, outputB_sum = 0;

    for (int i=0; i<num_channels;i++){
      outputA_sum += (double)frame[outputA_idx+i];
      outputB_sum += (double)frame[outputB_idx+i];
    }

    double vis_elem = (outputA_sum - outputB_sum)/(outputA_sum + outputB_sum);
    vis_elem *= vis_elem;

    vis_arr.push_back(vis_elem);

    cout << "FINISHED MY FFTS with vis: " << vis_elem  << endl;

    if ((vis_elem) > lock_vis){
        return 0;
    }

    return 1;
}
