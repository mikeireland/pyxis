
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
struct FineMetrology: FLIRCameraServer{

    FineMetrology() : FLIRCameraServer(AnotherCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<FineMetrology>("FM")
        // To insterface a class method, you can use the `def` method.
        .def("status", &FineMetrology::status, "Camera Status")
        .def("connect", &FineMetrology::connectcam, "Connect the camera")
        .def("disconnect", &FineMetrology::disconnectcam, "Disconnect the camera")
        .def("start", &FineMetrology::startcam, "Start exposures")
        .def("stop", &FineMetrology::stopcam, "Stop exposures")
        .def("getlatestfilename", &FineMetrology::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FineMetrology::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &FineMetrology::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &FineMetrology::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &FineMetrology::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &FineMetrology::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &FineMetrology::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &FineMetrology::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &FineMetrology::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &FineMetrology::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &FineMetrology::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &FineMetrology::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &FineMetrology::getparams, "Get all parameters");

}
