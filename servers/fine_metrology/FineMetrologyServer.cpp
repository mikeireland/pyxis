
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"


// Return 1 if error!
int NoCallback (unsigned short* data){
    return 0;
}


// FLIR Camera Server
struct FineMetrology: FLIRCameraServer{

    FineMetrology() : FLIRCameraServer(NoCallback){
    }

};

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<FineMetrology>("FM")
        .def("status", &FineMetrology::status, "Camera Status")
        .def("connect", &FineMetrology::connectcam, "Connect the camera")
        .def("disconnect", &FineMetrology::disconnectcam, "Disconnect the camera")
        .def("start", &FineMetrology::startcam, "Start exposures [number of frames, coadd flag]")
        .def("stop", &FineMetrology::stopcam, "Stop exposures")
        .def("getlatestfilename", &FineMetrology::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FineMetrology::getlatestimage, "Get the latest image data [binning flag, compression parameter]")
        .def("reconfigure_all", &FineMetrology::reconfigure_all, "Reconfigure all parameters [configuration struct as a json]")
        .def("reconfigure_gain", &FineMetrology::reconfigure_gain, "Reconfigure the gain [gain]")
        .def("reconfigure_exptime", &FineMetrology::reconfigure_exptime, "Reconfigure the exposure time [exptime in us]")
        .def("reconfigure_width", &FineMetrology::reconfigure_width, "Reconfigure the width [width in px]")
        .def("reconfigure_height", &FineMetrology::reconfigure_height, "Reconfigure the height [height in px]")
        .def("reconfigure_offsetX", &FineMetrology::reconfigure_offsetX, "Reconfigure the X offset [xoffset in px]")
        .def("reconfigure_offsetY", &FineMetrology::reconfigure_offsetY, "Reconfigure the Y offset [yoffset in px]")
        .def("reconfigure_blacklevel", &FineMetrology::reconfigure_blacklevel, "Reconfigure the black level [black_level]")
        .def("reconfigure_buffersize", &FineMetrology::reconfigure_buffersize, "Reconfigure the buffer size [buffer size in frames]")
        .def("reconfigure_savedir", &FineMetrology::reconfigure_savedir, "Reconfigure the save directory [save directory as a string]")
        .def("getparams", &FineMetrology::getparams, "Get all parameters");

}
