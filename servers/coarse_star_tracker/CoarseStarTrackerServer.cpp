
#include <fmt/core.h>
#include <iostream>
#include <fstream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"

//PLATE SOLVER SENDS ZMQ REQUEST OF GET LATEST FILENAME TO PLATESOLVE. THAT'S ABOUT ALL THE INTERACTIONS!

/*
Callback function to do nothing!
Inputs:
    data - array of the raw camera data
Output:
    return 1 if error
*/
int NoCallback (unsigned short* data){
    return 0;
}

// FLIR Camera Server
struct CoarseStarTracker: FLIRCameraServer{

    CoarseStarTracker() : FLIRCameraServer(NoCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<CoarseStarTracker>("CST")
        .def("status", &CoarseStarTracker::status, "Camera Status")
        .def("connect", &CoarseStarTracker::connectcam, "Connect the camera")
        .def("disconnect", &CoarseStarTracker::disconnectcam, "Disconnect the camera")
        .def("start", &CoarseStarTracker::startcam, "Start exposures [number of frames, coadd flag]")
        .def("stop", &CoarseStarTracker::stopcam, "Stop exposures")
        .def("getlatestfilename", &CoarseStarTracker::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &CoarseStarTracker::getlatestimage, "Get the latest image data [binning flag, compression parameter]")
        .def("reconfigure_all", &CoarseStarTracker::reconfigure_all, "Reconfigure all parameters [configuration struct as a json]")
        .def("reconfigure_gain", &CoarseStarTracker::reconfigure_gain, "Reconfigure the gain [gain]")
        .def("reconfigure_exptime", &CoarseStarTracker::reconfigure_exptime, "Reconfigure the exposure time [exptime in us]")
        .def("reconfigure_width", &CoarseStarTracker::reconfigure_width, "Reconfigure the width [width in px]")
        .def("reconfigure_height", &CoarseStarTracker::reconfigure_height, "Reconfigure the height [height in px]")
        .def("reconfigure_offsetX", &CoarseStarTracker::reconfigure_offsetX, "Reconfigure the X offset [xoffset in px]")
        .def("reconfigure_offsetY", &CoarseStarTracker::reconfigure_offsetY, "Reconfigure the Y offset [yoffset in px]")
        .def("reconfigure_blacklevel", &CoarseStarTracker::reconfigure_blacklevel, "Reconfigure the black level [black_level]")
        .def("reconfigure_buffersize", &CoarseStarTracker::reconfigure_buffersize, "Reconfigure the buffer size [buffer size in frames]")
        .def("reconfigure_savedir", &CoarseStarTracker::reconfigure_savedir, "Reconfigure the save directory [save directory as a string]")
        .def("getparams", &CoarseStarTracker::getparams, "Get all parameters");
        
}
