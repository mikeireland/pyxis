
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "QHYcamServerFuncs.h"


// Return 1 if error!
int AnotherCallback (unsigned short* data){
    cout << "I'm not working here!" << endl;
    return 0;
}


// FLIR Camera Server
struct SciCam: QHYCameraServer{

    SciCam() : QHYCameraServer(AnotherCallback){
    }

};

// Register as commander server
// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<SciCam>("SC")
        // To insterface a class method, you can use the `def` method.
        .def("status", &SciCam::status, "Camera Status")
        .def("connect", &SciCam::connectcam, "Connect the camera")
        .def("disconnect", &SciCam::disconnectcam, "Disconnect the camera")
        .def("start", &SciCam::startcam, "Start exposures")
        .def("stop", &SciCam::stopcam, "Stop exposures")
        .def("getlatestfilename", &SciCam::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &SciCam::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &SciCam::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &SciCam::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &SciCam::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &SciCam::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &SciCam::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &SciCam::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &SciCam::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &SciCam::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &SciCam::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &SciCam::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &SciCam::getparams, "Get all parameters");

}
