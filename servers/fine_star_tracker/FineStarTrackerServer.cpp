
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "FLIRcamServerFuncs.h"
#include <pthread.h>
#include "globals.h"
#include "image.hpp"
#include <opencv2/opencv.hpp>


using json = nlohmann::json;

struct centroid {
    double x;
    double y;
};

centroid GLOB_FST_CENTROID;

pthread_mutex_t GLOB_FST_FLAG_LOCK;

namespace nlohmann {
    template <>
    struct adl_serializer<centroid> {
        static void to_json(json& j, const centroid& c) {
            j = json{{"x", c.x},{"y", c.y}};
        }

        static void from_json(const json& j, centroid& c) {
            j.at("x").get_to(c.x);
            j.at("y").get_to(c.y);
        }
    };
}

centroid CalcStarPosition(cv::Mat img){

    // Function to take image array and find the star position
    //static image::ImageProcessSubMatInterp ipb;
    //auto p = ipb(img, dark);
    
    centroid result;
    result.x = x;
    result.y = y;

    return result;

}


// Return 1 if error!
int FST_Callback (unsigned short* data){

    int height = GLOB_IMSIZE/GLOB_WIDTH;

    cv::Mat img (height,GLOB_WIDTH,CV_16U,data);

    centroid position = CalcStarPosition(img);

    pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
    GLOB_FST_CENTROID = position;
    pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);

    //ZMQ CLIENT SEND TO CHIEF ROBOT position


    return 0;
}


// FLIR Camera Server
struct FineStarTracker: FLIRCameraServer{

    FineStarTracker() : FLIRCameraServer(FSTCallback){
    }

    centroid getstarposition(){
    centroid ret_position;
    pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
    ret_position = GLOB_FST_CENTROID;
    pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);
    return ret_position;
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
        .def("getparams", &FineStarTracker::getparams, "Get all parameters")
        .def("getstar", &FineStarTracker::getstarposition, "Get position of the star");

}
