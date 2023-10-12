#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "toml.hpp"
#include "FLIRcamServerFuncs.h"
#include <pthread.h>
#include <fstream>
#include "globals.h"
#include "centroid.hpp"

#include <opencv2/opencv.hpp>
#include <unistd.h>

#include <cstdlib>

using json = nlohmann::json;

// Centroid structs of measurement and target
centroid GLOB_FST_CENTROID;
centroid GLOB_FST_TARGET_CENTROID;

// Other global variables
int GLOB_FST_CENTROID_EXPTIME;
int GLOB_FST_PLATESOLVE_EXPTIME;

double GLOB_FST_PLATESCALE;

int GLOB_FST_CENTROID_FLAG = 0;

pthread_mutex_t GLOB_FST_FLAG_LOCK;

std::string GLOB_RB_TCP = "NOFILESAVED";

commander::client::Socket* RB_SOCKET;

// Serialise centroid struct into JSON
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

/*
Function to calculate the (brightest) star position in a given image
Inputs:
    img - OpenCV image matrix
    height - height of image in pixels
    width - width of image in pixels
Output:
    returns a centroid of the star position
*/
centroid CalcStarPosition(cv::Mat img, int height, int width){

    // Create the window of the entire image
    auto window = cv::Rect(0, 0, width, height);

    // Function to take image array and find the star position
    auto p = centroid_funcs::windowCentroidCOG(img, 7, 9, window);
    
    centroid result;
    
    result.x = p.x;
    result.y = p.y;

    return result;
}

/*
Callback function to either calculate and send the star's position if on centroid mode,
else it will do nothing (as it is instead plate solving)
Inputs:
    data - array of the raw camera data
Output:
    return 1 if error
*/
int FST_Callback (unsigned short* data){

    // If centroid mode is on, do stuff
    if (GLOB_FST_CENTROID_FLAG){

        int height = GLOB_IMSIZE/GLOB_WIDTH;
        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);

        // Get star position
        centroid position = CalcStarPosition(img,height,GLOB_WIDTH);

        // Calculate differential position from target (based on platescale)
        centroid diff_angles;
        diff_angles.x = (position.x - GLOB_FST_TARGET_CENTROID.x)*GLOB_FST_PLATESCALE;
        diff_angles.y = (position.y - GLOB_FST_TARGET_CENTROID.y)*GLOB_FST_PLATESCALE;

        pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
        GLOB_FST_CENTROID = diff_angles;
        pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);

        // Send the differential positions to the robot
        cout << "Sending diff angles" << endl;
        cout << diff_angles.x << ", " << diff_angles.y << endl;
        RB_SOCKET->send<std::string>("RC.receive_ST_angles", diff_angles.x, diff_angles.y, 0.0);

    }

    return 0;
}


// FLIR Camera Server for the fine star tracker
struct FineStarTracker: FLIRCameraServer{

    FineStarTracker() : FLIRCameraServer(FST_Callback){
    
         // Set up client parameters
        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string RB_port = config["FineStarTracker"]["RB_port"].value_or("4000");
        std::string IP = config["IP"].value_or("192.168.1.4");

        // Turn into a TCPString
        GLOB_RB_TCP = "tcp://" + IP + ":" + RB_port;
        
        // Create robot socket
        RB_SOCKET = new commander::client::Socket(GLOB_RB_TCP);
        
        GLOB_FST_PLATESCALE = config["FineStarTracker"]["platescale"].value_or(1.0);

        GLOB_FST_TARGET_CENTROID.x = config["FineStarTracker"]["centroid_x_target"].value_or(1500.0);
        GLOB_FST_TARGET_CENTROID.y = config["FineStarTracker"]["centroid_y_target"].value_or(1000.0);

        GLOB_FST_CENTROID_EXPTIME = config["FineStarTracker"]["Centroid_exptime"].value_or(1000);
        GLOB_FST_PLATESOLVE_EXPTIME = config["FineStarTracker"]["PlateSolve_exptime"].value_or(1000);

    }
    
    ~FineStarTracker(){
        delete RB_SOCKET;
    }

    /*
    Function to get the current differential star position as a centroid struct
    */
    centroid getstarposition(){
        centroid ret_position;
        pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
        ret_position = GLOB_FST_CENTROID;
        pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);
        return ret_position;
    }

    /*
    Function to switch the solving mode to CENTROIDING (i.e centroid without saving image)
    */
    string switchToCentroid(){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                
                // First, stop the camera if running
                ret_msg = this->stopcam();
                cout << ret_msg << endl;
                // Reconfigure the exposure time to be lower
                ret_msg = this->reconfigure_exptime(GLOB_FST_CENTROID_EXPTIME);
                cout << ret_msg << endl;
                // Start the camera to not save images
                pthread_mutex_lock(&GLOB_FLAG_LOCK);
                GLOB_NUMFRAMES = 0;
                GLOB_FST_CENTROID_FLAG = 1;
                pthread_mutex_unlock(&GLOB_FLAG_LOCK);
                ret_msg = this->startcam(GLOB_NUMFRAMES,GLOB_COADD);
                cout << ret_msg << endl;

                ret_msg = "Switched to Centroiding Mode";
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    /*
    Function to switch the solving mode to PLATE SOLVING (i.e just save images)
    */
    string switchToPlatesolve(){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                // First, stop the camera if running
                ret_msg = this->stopcam();
                cout << ret_msg << endl;
                // Reconfigure the exposure time to be higher
                ret_msg = this->reconfigure_exptime(GLOB_FST_PLATESOLVE_EXPTIME);
                cout << ret_msg << endl;
                // Start the camera and save images
                pthread_mutex_lock(&GLOB_FLAG_LOCK);
                GLOB_NUMFRAMES = 1;
                GLOB_FST_CENTROID_FLAG = 0;
                pthread_mutex_unlock(&GLOB_FLAG_LOCK);
                ret_msg = this->startcam(GLOB_NUMFRAMES,GLOB_COADD);
                cout << ret_msg << endl;
                
                ret_msg = "Switched to Plate Solving Mode";
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
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
        .def("start", &FineStarTracker::startcam, "Start exposures [number of frames, coadd flag]")
        .def("stop", &FineStarTracker::stopcam, "Stop exposures")
        .def("getlatestfilename", &FineStarTracker::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FineStarTracker::getlatestimage, "Get the latest image data [binning flag, compression parameter]")
        .def("reconfigure_all", &FineStarTracker::reconfigure_all, "Reconfigure all parameters [configuration struct as a json]")
        .def("reconfigure_gain", &FineStarTracker::reconfigure_gain, "Reconfigure the gain [gain]")
        .def("reconfigure_exptime", &FineStarTracker::reconfigure_exptime, "Reconfigure the exposure time [exptime in us]")
        .def("reconfigure_width", &FineStarTracker::reconfigure_width, "Reconfigure the width [width in px]")
        .def("reconfigure_height", &FineStarTracker::reconfigure_height, "Reconfigure the height [height in px]")
        .def("reconfigure_offsetX", &FineStarTracker::reconfigure_offsetX, "Reconfigure the X offset [xoffset in px]")
        .def("reconfigure_offsetY", &FineStarTracker::reconfigure_offsetY, "Reconfigure the Y offset [yoffset in px]")
        .def("reconfigure_blacklevel", &FineStarTracker::reconfigure_blacklevel, "Reconfigure the black level [black_level]")
        .def("reconfigure_buffersize", &FineStarTracker::reconfigure_buffersize, "Reconfigure the buffer size [buffer size in frames]")
        .def("reconfigure_savedir", &FineStarTracker::reconfigure_savedir, "Reconfigure the save directory [save directory as a string]")
        .def("getparams", &FineStarTracker::getparams, "Get all parameters")
        .def("getstar", &FineStarTracker::getstarposition, "Get position of the star")
        .def("switchCentroid", &FineStarTracker::switchToCentroid, "Switch to Centroiding Mode")
        .def("switchPlateSolve", &FineStarTracker::switchToPlatesolve, "Switch to Plate Solving Mode");

}
