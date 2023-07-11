
#include <fmt/core.h>
#include <iostream>
#include <fstream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"

//PLATE SOLVER SENDS ZMQ REQUEST OF GET LATEST FILENAME TO PLATESOLVE. THAT'S ABOUT ALL THE INTERACTIONS!

// Return 1 if error!
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
        // To insterface a class method, you can use the `def` method.
        .def("status", &CoarseStarTracker::status, "Camera Status")
        .def("connect", &CoarseStarTracker::connectcam, "Connect the camera")
        .def("disconnect", &CoarseStarTracker::disconnectcam, "Disconnect the camera")
        .def("start", &CoarseStarTracker::startcam, "Start exposures")
        .def("stop", &CoarseStarTracker::stopcam, "Stop exposures")
        .def("getlatestfilename", &CoarseStarTracker::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &CoarseStarTracker::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &CoarseStarTracker::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &CoarseStarTracker::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &CoarseStarTracker::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &CoarseStarTracker::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &CoarseStarTracker::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &CoarseStarTracker::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &CoarseStarTracker::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &CoarseStarTracker::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &CoarseStarTracker::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &CoarseStarTracker::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &CoarseStarTracker::getparams, "Get all parameters");

}
