
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"


// Return 1 if error!
int AnotherCallback (unsigned short* data){
    cout << "I'm not working here!" << endl;
    return 0;
}


// FLIR Camera Server
struct FineStarTracker: FLIRCameraServer{

    FineStarTracker() : FLIRCameraServer(AnotherCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<FineStarTracker>("FST")
        // To insterface a class method, you can use the `def` method.
        .def("status", &FineStarTracker::status, "Camera Status")
        .def("connect", &FineStarTracker::connectcam, "Connect the camera")
        .def("disconnect", &FineStarTracker::disconnectcam, "Disconnect the camera")
        .def("start", &FineStarTracker::startcam, "Start exposures")
        .def("stop", &FineStarTracker::stopcam, "Stop exposures")
        .def("getlatestfilename", &FineStarTracker::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FineStarTracker::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &FineStarTracker::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &FineStarTracker::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &FineStarTracker::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &FineStarTracker::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &FineStarTracker::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &FineStarTracker::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &FineStarTracker::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &FineStarTracker::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &FineStarTracker::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &FineStarTracker::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &FineStarTracker::getparams, "Get all parameters");

}
