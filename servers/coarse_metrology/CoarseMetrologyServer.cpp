
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
struct CoarseMet: FLIRCameraServer{

    CoarseMet() : FLIRCameraServer(AnotherCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<CoarseMet>("CM")
        // To insterface a class method, you can use the `def` method.
        .def("status", &CoarseMet::status, "Camera Status")
        .def("connect", &CoarseMet::connectcam, "Connect the camera")
        .def("disconnect", &CoarseMet::disconnectcam, "Disconnect the camera")
        .def("start", &CoarseMet::startcam, "Start exposures")
        .def("stop", &CoarseMet::stopcam, "Stop exposures")
        .def("getlatestfilename", &CoarseMet::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &CoarseMet::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &CoarseMet::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &CoarseMet::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &CoarseMet::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &CoarseMet::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &CoarseMet::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &CoarseMet::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &CoarseMet::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &CoarseMet::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &CoarseMet::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &CoarseMet::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &CoarseMet::getparams, "Get all parameters");

}
