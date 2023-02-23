
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
struct SimpleQHYCam: QHYCameraServer{

    SimpleQHYCam() : QHYCameraServer(AnotherCallback){
    }

    void newFunc()
    {
        cout << "New Func Here" << endl;
    }

};

// Register as commander server
// Register as commander server
COMMANDER_REGISTER(m)
{

    m.instance<SimpleQHYCam>("QHY")
        // To insterface a class method, you can use the `def` method.
        .def("status", &SimpleQHYCam::status, "Camera Status")
        .def("connect", &SimpleQHYCam::connectcam, "Connect the camera")
        .def("disconnect", &SimpleQHYCam::disconnectcam, "Disconnect the camera")
        .def("start", &SimpleQHYCam::startcam, "Start exposures")
        .def("stop", &SimpleQHYCam::stopcam, "Stop exposures")
        .def("getlatestfilename", &SimpleQHYCam::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &SimpleQHYCam::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &SimpleQHYCam::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &SimpleQHYCam::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &SimpleQHYCam::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &SimpleQHYCam::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &SimpleQHYCam::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &SimpleQHYCam::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &SimpleQHYCam::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &SimpleQHYCam::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &SimpleQHYCam::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &SimpleQHYCam::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &SimpleQHYCam::getparams, "Get all parameters")
        .def("newFunc", &SimpleQHYCam::newFunc, "TEST");

}
