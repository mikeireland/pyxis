
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"
#include "globals.h"
#include <opencv2/opencv.hpp>

// Return 1 if error!
int AnotherCallback (unsigned short* data){
    
    int height = GLOB_IMSIZE/GLOB_WIDTH;

    cv::Mat img (height,GLOB_WIDTH,CV_16U,data);

    //Function to do *SOMETHING* with the Fiber injection camera data; use imageproc?

    //ZMQ CLIENT SEND TO CHIEF ROBOT

    return 0;
}

// Calculate differential voltage corresponding to the given displacement
double displacementToVoltage(double displacement){
    //Linear??
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    double a = GLOB_FI_VOLTAGE_FACTOR;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
   
    double result = a*displacement;
    
    return result;
}


// FLIR Camera Server
struct FiberInjection: FLIRCameraServer{

    FiberInjection() : FLIRCameraServer(AnotherCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<FiberInjection>("FI")
        // To insterface a class method, you can use the `def` method.
        .def("status", &FiberInjection::status, "Camera Status")
        .def("connect", &FiberInjection::connectcam, "Connect the camera")
        .def("disconnect", &FiberInjection::disconnectcam, "Disconnect the camera")
        .def("start", &FiberInjection::startcam, "Start exposures")
        .def("stop", &FiberInjection::stopcam, "Stop exposures")
        .def("getlatestfilename", &FiberInjection::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FiberInjection::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &FiberInjection::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &FiberInjection::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &FiberInjection::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &FiberInjection::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &FiberInjection::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &FiberInjection::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &FiberInjection::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &FiberInjection::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &FiberInjection::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &FiberInjection::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &FiberInjection::getparams, "Get all parameters");

}
