
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
#include <unistd.h>

#include <cstdlib>
#include "centroiders.hpp"

using json = nlohmann::json;

struct centroid {
    double x;
    double y;
};

centroid GLOB_FST_CENTROID;

int GLOB_FST_START;
int GLOB_FST_STOP;

int GLOB_FST_CENTROID_EXPTIME;
int GLOB_FST_CENTROID_SLEEPTIME;
int GLOB_FST_PLATESOLVE_EXPTIME;
int GLOB_FST_PLATESOLVE_SLEEPTIME;


pthread_mutex_t GLOB_FST_FLAG_LOCK;

std::string GLOB_RB_TCP = "NOFILESAVED";

commander::client::Socket* RB_SOCKET;

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
    static image::ImageProcessSubMatInterpSingle ipb;

    auto p = ipb(img);
    
    cv::Mat thr;
    
    // convert grayscale to binary image
    cv::threshold( img, thr, 100,65535,cv::THRESH_BINARY );
 
    // find moments of the image
    cv::Moments m = cv::moments(thr,true);
    cv::Point pt(m.m10/m.m00, m.m01/m.m00);
    
    cout << cv::Mat(pt) << endl;
    
    centroid result;
    //result.x = rand();
    //result.y = rand();

    return result;

}

// Return 1 if error!
int FST_Callback (unsigned short* data){

    int height = GLOB_IMSIZE/GLOB_WIDTH;

    

    
    
    cv::Mat img (height,GLOB_WIDTH,CV_16U,data);
    img.convertTo(img, CV_8U, 0.00390625);
    uchar * img2 = img.isContinuous()? img.data: img.clone().data;

    lost::CenterOfGravityAlgorithm CA;

    lost::Stars s = CA.Go(img2,3072, 2048);

    //centroid position = CalcStarPosition(img);

    centroid position;
    position.x = s[0].position.x;
    position.y = s[0].position.y;

    pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
    GLOB_FST_CENTROID = position;
    pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);

    cout << "Sending position" << endl;
    
    cout << position.x << ", " << position.y << endl;
    //std::string result = RB_SOCKET->send<std::string>("test", position);

    return 0;
}


// FLIR Camera Server
struct FineStarTracker: FLIRCameraServer{

    FineStarTracker() : FLIRCameraServer(FST_Callback){
    
         // Set up client parameters
        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string RB_port = config["FineStarTracker"]["RB_port"].value_or("4000");
        std::string IP = config["IP"].value_or("192.168.1.4");

        // Turn into a TCPString
        GLOB_RB_TCP = "tcp://" + IP + ":" + RB_port;
        
        RB_SOCKET = new commander::client::Socket(GLOB_RB_TCP);
        
        
        GLOB_FST_CENTROID_EXPTIME = config["FineStarTracker"]["Centroid_exptime"].value_or(1000);
        GLOB_FST_CENTROID_SLEEPTIME = config["FineStarTracker"]["Centroid_sleeptime"].value_or(1000);
        GLOB_FST_PLATESOLVE_EXPTIME = config["FineStarTracker"]["PlateSolve_exptime"].value_or(1000);
        GLOB_FST_PLATESOLVE_SLEEPTIME = config["FineStarTracker"]["PlateSolve_sleeptime"].value_or(1000);
    
    }
    
    ~FineStarTracker(){
        delete RB_SOCKET;
    }

    centroid getstarposition(){
        centroid ret_position;
        pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
        ret_position = GLOB_FST_CENTROID;
        pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);
        return ret_position;
    }
    
    void *FST_Loop(void)
    {
        pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
        GLOB_FST_START = 1;
        pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);
        
        string ret_msg;
        
        while (GLOB_FST_STOP==0){
        
            cout << "WE PLATE SOLVING" << endl;
            pthread_mutex_lock(&GLOB_FLAG_LOCK);
            GLOB_NUMFRAMES = 1;
            pthread_mutex_unlock(&GLOB_FLAG_LOCK);
            ret_msg = this->startcam(GLOB_NUMFRAMES,GLOB_COADD);
            cout << ret_msg << endl;
            usleep(GLOB_FST_PLATESOLVE_SLEEPTIME);
            ret_msg = this->stopcam();
            while (GLOB_RUNNING == 1){
                usleep(1000);
            }
            cout << ret_msg << endl;
            ret_msg = this->reconfigure_exptime(GLOB_FST_CENTROID_EXPTIME);
            while (GLOB_RECONFIGURE == 1){
                usleep(1000);
            }
            cout << ret_msg << endl;
            
            cout << "WE CENTROIDING" << endl;
            pthread_mutex_lock(&GLOB_FLAG_LOCK);
            GLOB_NUMFRAMES = 0;
            pthread_mutex_unlock(&GLOB_FLAG_LOCK);
            ret_msg = this->startcam(GLOB_NUMFRAMES,GLOB_COADD);
            cout << ret_msg << endl;
            usleep(GLOB_FST_CENTROID_SLEEPTIME);
            ret_msg = this->stopcam();
            while (GLOB_RUNNING == 1){
                usleep(1000);
            }
            cout << ret_msg << endl;
            ret_msg = this->reconfigure_exptime(GLOB_FST_PLATESOLVE_EXPTIME);
            while (GLOB_RECONFIGURE == 1){
                usleep(1000);
            }
            cout << ret_msg << endl;
            //wait 220ms
            
        }
        pthread_mutex_lock(&GLOB_FST_FLAG_LOCK);
        GLOB_FST_START = 0;
        pthread_mutex_unlock(&GLOB_FST_FLAG_LOCK);
        pthread_exit(NULL);
    }
    
    static void *FST_Loop_helper(void *context)
    {
        return ((FineStarTracker *)context)->FST_Loop();
    }
    
    string startFSTloop(int num_frames, int coadd_flag){
        string ret_msg;
        
        if(GLOB_CAM_STATUS == 2){
		    if(GLOB_RUNNING == 0){
			    if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
			        if(GLOB_FST_START==0){
			            if(coadd_flag == 1 and num_frames > 50){
				            ret_msg = "Too many frames to coadd! Maximum is 50";
				        } else{
				            pthread_mutex_lock(&GLOB_FLAG_LOCK);
				            GLOB_NUMFRAMES = num_frames;
				            GLOB_COADD = coadd_flag;
				            //START FST LOOP
				            GLOB_FST_STOP=0;
				            pthread_create(&GLOB_CAMTHREAD, NULL, &FineStarTracker::FST_Loop_helper, this);
				            
				            pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				            ret_msg = "Starting Camera Exposures";
                        }
                    }else{
				        ret_msg = "Loop already running!";
				    }    
			    }else{
				    ret_msg = "Camera Busy!";
			    }
		    }else{
			    ret_msg = "Camera already running!";
		    }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }

	    return ret_msg;
	}    
	    
    // Stop acquisition of the camera
    string stopFSTloop(){
	    string ret_msg;
	    if(GLOB_CAM_STATUS == 2){
		    if(GLOB_FST_START == 1){
			    pthread_mutex_lock(&GLOB_FLAG_LOCK);
			    GLOB_FST_STOP = 1;
			    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
			    
			    pthread_join(GLOB_CAMTHREAD, NULL);
			    
			    ret_msg = "Stopping Camera Exposures";
		    }else{
			    ret_msg = "Loop not running!";
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
        .def("getstar", &FineStarTracker::getstarposition, "Get position of the star")
        .def("startFST", &FineStarTracker::startFSTloop, "Start FST Loop")
        .def("stopFST", &FineStarTracker::stopFSTloop, "Stop FST Loop");

}
