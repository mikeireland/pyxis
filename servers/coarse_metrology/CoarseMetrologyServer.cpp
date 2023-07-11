
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "toml.hpp"
#include "FLIRcamServerFuncs.h"
#include <pthread.h>
#include "globals.h"
#include "image.hpp"
#include <opencv2/opencv.hpp>


using json = nlohmann::json;

struct LEDs {
    double LED1_x;
    double LED1_y;
    double LED2_x;
    double LED2_y;
};

LEDs GLOB_CM_LEDs;
int GLOB_CM_ONFLAG = 0;
int GLOB_CM_ENABLEFLAG = 0;

cv::Mat GLOB_CM_IMG_DARK;

pthread_mutex_t GLOB_CM_FLAG_LOCK;
pthread_mutex_t GLOB_CM_IMG_LOCK;

std::string GLOB_RB_TCP = "NOFILESAVED";
std::string GLOB_DA_TCP = "NOFILESAVED";

commander::client::Socket* RB_SOCKET;
commander::client::Socket* DA_SOCKET;

namespace nlohmann {
    template <>
    struct adl_serializer<LEDs> {
        static void to_json(json& j, const LEDs& L) {
            j = json{{"LED1_x", L.LED1_x},{"LED1_y", L.LED1_y}, 
            {"LED2_x", L.LED2_x},{"LED2_y", L.LED2_y}};
        }

        static void from_json(const json& j, LEDs& L) {
            j.at("LED1_x").get_to(L.LED1_x);
            j.at("LED1_y").get_to(L.LED1_y);
            j.at("LED2_x").get_to(L.LED2_x);
            j.at("LED2_y").get_to(L.LED2_y);
        }
    };
}

LEDs CalcLEDPosition(cv::Mat img, cv::Mat dark){

    // Function to take image array and find the two LED positions
    static image::ImageProcessSubMatInterp ipb;
    auto p = ipb(img, dark);
    
    LEDs result;
    result.LED1_x = p.p1.x;
    result.LED1_y = p.p1.y;
    result.LED2_x = p.p2.x;
    result.LED2_y = p.p2.y;

    return result;

}


// Return 1 if error!
int CM_Callback (unsigned short* data){

    if (GLOB_CM_ENABLEFLAG){
        int height = GLOB_IMSIZE/GLOB_WIDTH;

        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);

        //If LED is ON
        if (GLOB_CM_ONFLAG){
            
            cout << "LED On" << endl;
            pthread_mutex_lock(&GLOB_CM_IMG_LOCK);
            LEDs positions = CalcLEDPosition(img,GLOB_CM_IMG_DARK);
            pthread_mutex_unlock(&GLOB_CM_IMG_LOCK);
            pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
            GLOB_CM_LEDs = positions;
            pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);
            
            //ZMQ CLIENT SEND TO DEPUTY ROBOT positions
            //std::string result = RB_SOCKET->send<std::string>("RC.receive_LED_positions", positions);
            
            //ZMQ CLIENT SEND TO AUX TURN OFF LED
            std::string result = DA_SOCKET->send<std::string>("DA.LEDOff");

        
            pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
            GLOB_CM_ONFLAG = 0;
            pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);

        }

        else {
        
            cout << "LED Off" << endl;
            pthread_mutex_lock(&GLOB_CM_IMG_LOCK);
            img.copyTo(GLOB_CM_IMG_DARK);
            pthread_mutex_unlock(&GLOB_CM_IMG_LOCK);

            //ZMQ CLIENT SEND TO AUX TURN ON LED
            std::string result = DA_SOCKET->send<std::string>("DA.LEDOn");

            pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
            GLOB_CM_ONFLAG = 1;
            pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);

        }
    }
    return 0;
}


// FLIR Camera Server
struct CoarseMet: FLIRCameraServer{

    CoarseMet() : FLIRCameraServer(CM_Callback){
    
        // Set up client parameters
        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string RB_port = config["CoarseMet"]["RB_port"].value_or("4000");
        std::string DA_port = config["CoarseMet"]["DA_port"].value_or("4000");
        std::string IP = config["CoarseMet"]["IP"].value_or("192.168.1.4");

        // Turn into a TCPString
        GLOB_RB_TCP = "tcp://" + IP + ":" + RB_port;
        GLOB_DA_TCP = "tcp://" + IP + ":" + DA_port;
        
        RB_SOCKET = new commander::client::Socket(GLOB_RB_TCP);
        DA_SOCKET = new commander::client::Socket(GLOB_DA_TCP);
    
    }
    
    ~CoarseMet(){
        delete RB_SOCKET;
        delete DA_SOCKET;
    }

    LEDs getLEDpositions(){
        LEDs ret_LEDs;
        pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
        ret_LEDs = GLOB_CM_LEDs;
        pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);
        return ret_LEDs;
    }

    string enableCoarseMetLEDs(int flag){
        string ret_msg = "Changing enable flag to: " + to_string(flag);
        pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
        GLOB_CM_ENABLEFLAG = flag;
        pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);
        return ret_msg;
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
        .def("getparams", &CoarseMet::getparams, "Get all parameters")
        .def("getLEDs", &CoarseMet::getLEDpositions, "Get positions of two LEDs")
        .def("enableLEDs", &CoarseMet::enableCoarseMetLEDs, "Enable the blinking and measuring loop");

}
