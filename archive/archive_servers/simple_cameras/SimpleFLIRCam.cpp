
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
struct SimpleFLIRCam: FLIRCameraServer{

    SimpleFLIRCam() : FLIRCameraServer(AnotherCallback){
    }

    void newFunc()
    {
        cout << "New Func Here" << endl;
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<SimpleFLIRCam>("FLIR")
        // To insterface a class method, you can use the `def` method.
        .def("status", &SimpleFLIRCam::status, "Camera Status")
        .def("connect", &SimpleFLIRCam::connectcam, "Connect the camera")
        .def("disconnect", &SimpleFLIRCam::disconnectcam, "Disconnect the camera")
        .def("start", &SimpleFLIRCam::startcam, "Start exposures")
        .def("stop", &SimpleFLIRCam::stopcam, "Stop exposures")
        .def("getlatestfilename", &SimpleFLIRCam::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &SimpleFLIRCam::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &SimpleFLIRCam::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &SimpleFLIRCam::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &SimpleFLIRCam::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &SimpleFLIRCam::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &SimpleFLIRCam::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &SimpleFLIRCam::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &SimpleFLIRCam::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &SimpleFLIRCam::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &SimpleFLIRCam::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &SimpleFLIRCam::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &SimpleFLIRCam::getparams, "Get all parameters")
        .def("newFunc", &SimpleFLIRCam::newFunc, "TEST");

}
