#ifndef _FLIRCamServer_
#define _FLIRCamServer_

#include <string>
#include "globals.h"

using namespace std;

// FLIR Camera Server
struct FLIRCameraServer {

    FLIRCameraServer();
    
    ~FLIRCameraServer();


//Get status of camera
string status();

// Get parameters from the global variable
configuration getparams();

// Reconfigure parameters from an input configuration struct
string reconfigure_all(configuration c);


/* ##################################################### */

/* Set each parameter separately: */
string reconfigure_gain(float gain);

string reconfigure_exptime(float exptime);

string reconfigure_width(int width);

string reconfigure_height(int height);

string reconfigure_offsetX(int offsetX);

string reconfigure_offsetY(int offsetY);

string reconfigure_blacklevel(float blacklevel);

string reconfigure_buffersize(float buffersize);

string reconfigure_savedir(float savedir);

/* ##################################################### */

// Connect to a camera, by starting up the runCam pThread
string connectcam();

// Disconnect to a camera, by signalling and joining the runCam pThread
string disconnectcam();

// Start acquisition of the camera. Takes in the number of frames to save
// per FITS file (or 0 for continuous, no saving)
string startcam(int num_frames, int coadd_flag);

// Stop acquisition of the camera
string stopcam();

// Get the filename of the latest saved FITS image
string getlatestfilename();

// Get the latest image data from the camera thread
string getlatestimage(int compression, int binning);

};

#endif

