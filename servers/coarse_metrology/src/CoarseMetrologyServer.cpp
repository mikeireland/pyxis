
#include <fmt/core.h>
#include <iostream>
#include <fstream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "toml.hpp"
#include "FLIRcamServerFuncs.h"
#include <pthread.h>
#include "globals.h"
#include "image.hpp"
#include <opencv2/opencv.hpp>


using json = nlohmann::json;

//LED Struct for the x/y position of both LED positions
struct LEDs {
    double LED1_x = 0.; 
    double LED1_y = 0.;
    double LED2_x = 0.;
    double LED2_y = 0.;
};

LEDs GLOB_CM_LEDs; //Main LED struct
int GLOB_CM_ONFLAG = 0; //Are the LEDs on?
int GLOB_CM_ENABLEFLAG = 0; //Is the coarse metrology to be enabled?

cv::Mat GLOB_CM_IMG_DARK; //Dark image

pthread_mutex_t GLOB_CM_FLAG_LOCK; //Lock on changing flags
pthread_mutex_t GLOB_CM_IMG_LOCK; //Lock on the images

std::string GLOB_RB_TCP = "NOFILESAVED"; //Robot TCP address
std::string GLOB_DA_TCP = "NOFILESAVED"; //Dep Aux TCP address

commander::client::Socket* RB_SOCKET; //Robot commander socket
commander::client::Socket* DA_SOCKET; //Dep Aux commander socket

//Serialiser for commander
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


namespace nlohmann {
    template <>
    struct adl_serializer<cv::Point2d> {
        static void to_json(json& j, const cv::Point2d& p) {
            j = json{{"x", p.x}, {"y", p.y}};
        }
        static void from_json(const json& j, cv::Point2d& p) {
            j.at("x").get_to(p.x);
            j.at("y").get_to(p.y);
        }
    };
}

/* Function to calculate the LED positions from an image
Inputs:
    img - OpenCV image to process
    dark - OpenCV image of the system without LEDs on

Outputs:
    struct of LED positions
*/
LEDs CalcLEDPosition(cv::Mat img, cv::Mat dark){

    LEDs result;
    try {
        // Function to take image array and find the two LED positions
        static image::ImageProcessSubMatInterp ipb;

        auto p = ipb(img, dark);
        
        result.LED1_x = p.p1.x;
        result.LED1_y = p.p1.y;
        result.LED2_x = p.p2.x;
        result.LED2_y = p.p2.y;

    } catch (const cv::Exception& e) {
        std:cout << e.what() << std::endl;
        result.LED1_x = result.LED1_y = result.LED2_x = result.LED2_y = -1;
    }   

    return result;

}

/*
Callback function to calculate the metrology.
Inputs:
    data - array of the raw camera data
Output:
    return 1 if error
*/
int CM_Callback (unsigned short* data){

    if (GLOB_CM_ENABLEFLAG){
        int height = GLOB_IMSIZE/GLOB_WIDTH;

        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);
        cv::Mat img_float;
        img.convertTo(img_float, CV_32F);//Edit by Qianhui: convert to float for processing

        //If LED is ON
        if (GLOB_CM_ONFLAG){
            
            cv::Mat dark_float;
            GLOB_CM_IMG_DARK.convertTo(dark_float, CV_32F);//Edit by Qianhui: convert to float for processing

            cout << "LED On" << endl;
            pthread_mutex_lock(&GLOB_CM_IMG_LOCK);
            LEDs positions = CalcLEDPosition(img_float,dark_float);//Pass the float images to CalcLEDPosition function
            pthread_mutex_unlock(&GLOB_CM_IMG_LOCK);
            pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
            GLOB_CM_LEDs = positions;
            pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);
            
            cout << "LED1: (" << positions.LED1_x << ", " << positions.LED1_y << ")" << endl;
            cout << "LED2: (" << positions.LED2_x << ", " << positions.LED2_y << ")" << endl;

            // Log LED positions to file
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            std::ofstream logfile("/home/pyxisuser/pyxis/servers/coarse_metrology/data/led_positions.log", std::ios::app);
            if (logfile.is_open()) {
                logfile << std::fixed << std::setprecision(2)
                        << "LED1: " << positions.LED1_x << "," << positions.LED1_y << ","
                        << "LED2: " << positions.LED2_x << "," << positions.LED2_y << ","
                        << "Timestamp: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl; // Add timestamp
                logfile.close();
    }

            //ZMQ CLIENT SEND TO DEPUTY ROBOT positions
            //RB_SOCKET->send<int>("RC.receive_LED_positions", positions);
            
            //ZMQ CLIENT SEND TO AUX TURN OFF LED
            DA_SOCKET->send<int>("DA.LEDOff");

        
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
            DA_SOCKET->send<int>("DA.LEDOn");

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

    /*
    Gets the positions of both LEDs
    Outputs:
        Returns a struct of the x/y positions of the LEDs
    */
    LEDs getLEDpositions(){
        LEDs ret_LEDs;
        pthread_mutex_lock(&GLOB_CM_FLAG_LOCK);
        ret_LEDs = GLOB_CM_LEDs;
        pthread_mutex_unlock(&GLOB_CM_FLAG_LOCK);
        return ret_LEDs;
    }

    // Calculate the alignment error based on the LED positions and angles
    json getAlignmentError(){

        //Example of parameters, needs to be calibrated:
        double beta = 0.5;
        double gamma = 0.02;
        cv::Point2d x0 = cv::Point2d(0.1, 45.0);
        cv::Point2d alpha_c = cv::Point2d(0.002, -0.001);

        // Get LED positions
        LEDs leds = getLEDpositions();
        cv::Point2d LED1(leds.LED1_x, leds.LED1_y);
        cv::Point2d LED2(leds.LED2_x, leds.LED2_y);

        // Call the image library function
        auto err = image::compute_alignment_error(
            LED1, LED2, beta, gamma, x0, alpha_c,
            GLOB_WIDTH, GLOB_IMSIZE, GLOB_PIX_PER_RAD
        ); 

        // return alpha_1, alpha_2, dlt_p (all cv::Point2d)
        if (err.dlt_p == cv::Point2d(-1, -1) && 
            err.alpha_1 == cv::Point2d(0,0) && 
            err.alpha_2 == cv::Point2d(0,0)) {
            std::cerr << "Alignment error calculation failed. Please check if both LEDs are in sight." << std::endl;
        }

        json j;
        j["alpha_1"] = {{"x", err.alpha_1.x}, {"y", err.alpha_1.y}};
        j["alpha_2"] = {{"x", err.alpha_2.x}, {"y", err.alpha_2.y}};
        j["dlt_p"]   = {{"x", err.dlt_p.x},   {"y", err.dlt_p.y}};

        
        return j;
    }

    /*
    Function to enable the coarse metrology
    Input:
        flag to enable (1=on, 0=off)
    Output:
        String output message
    */
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
        .def("status", &CoarseMet::status, "Camera Status")
        .def("connect", &CoarseMet::connectcam, "Connect the camera")
        .def("disconnect", &CoarseMet::disconnectcam, "Disconnect the camera")
        .def("start", &CoarseMet::startcam, "Start exposures [number of frames, coadd flag]")
        .def("stop", &CoarseMet::stopcam, "Stop exposures")
        .def("getlatestfilename", &CoarseMet::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &CoarseMet::getlatestimage, "Get the latest image data [compression parameter, binning flag]")
        .def("reconfigure_all", &CoarseMet::reconfigure_all, "Reconfigure all parameters [configuration struct as a json]")
        .def("reconfigure_gain", &CoarseMet::reconfigure_gain, "Reconfigure the gain [gain]")
        .def("reconfigure_exptime", &CoarseMet::reconfigure_exptime, "Reconfigure the exposure time [exptime in us]")
        .def("reconfigure_width", &CoarseMet::reconfigure_width, "Reconfigure the width [width in px]")
        .def("reconfigure_height", &CoarseMet::reconfigure_height, "Reconfigure the height [height in px]")
        .def("reconfigure_offsetX", &CoarseMet::reconfigure_offsetX, "Reconfigure the X offset [xoffset in px]")
        .def("reconfigure_offsetY", &CoarseMet::reconfigure_offsetY, "Reconfigure the Y offset [yoffset in px]")
        .def("reconfigure_blacklevel", &CoarseMet::reconfigure_blacklevel, "Reconfigure the black level [black_level]")
        .def("reconfigure_buffersize", &CoarseMet::reconfigure_buffersize, "Reconfigure the buffer size [buffer size in frames]")
        .def("reconfigure_savedir", &CoarseMet::reconfigure_savedir, "Reconfigure the save directory [save directory as a string]")
        .def("getparams", &CoarseMet::getparams, "Get all parameters")
        .def("resetUSBPort", &CoarseMet::resetUSBPort, "Reset the USB port on the HUB [string HUB name, string port number]")
        .def("getLEDs", &CoarseMet::getLEDpositions, "Get positions of two LEDs")
        .def("getAlignmentError", &CoarseMet::getAlignmentError, "Calculate the alignment error based on LEDs in coarse metrology camera. Make sure the parameters are calibrated.")
        .def("enableLEDs", &CoarseMet::enableCoarseMetLEDs, "Enable the blinking and measuring loop [flag]");

}
