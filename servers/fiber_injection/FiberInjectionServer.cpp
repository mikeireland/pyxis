
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "FLIRcamServerFuncs.h"
#include "globals.h"
#include "toml.hpp"
#include "centroid.hpp"
#include <opencv2/opencv.hpp>

using json = nlohmann::json;

struct centroid {
    double x;
    double y;
};

centroid GLOB_FI_DEXTRA_TARGET_CENTROID;
centroid GLOB_FI_DEXTRA_CURRENT_CENTROID;
centroid GLOB_FI_DEXTRA_DIFF_POSITION;

centroid GLOB_FI_SINISTRA_TARGET_CENTROID;
centroid GLOB_FI_SINISTRA_CURRENT_CENTROID;
centroid GLOB_FI_SINISTRA_DIFF_POSITION;

pthread_mutex_t GLOB_FI_FLAG_LOCK;

commander::client::Socket* CA_SOCKET;

int GLOB_FI_ENABLECENTROID_FLAG;
int GLOB_FI_TIPTILTSERVO_FLAG;
int GLOB_FI_SET_TARGET_FLAG;

int GLOB_FI_INTERP_SIZE;
int GLOB_FI_GAUSS_RAD;
int GLOB_FI_WINDOW_SIZE;
int GLOB_FI_CENTROID_GAIN;

cv::Mat GLOB_FI_CENTROID_WEIGHTS;

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

// Return 1 if error!
int FibreInjectionCallback (unsigned short* data){
    if (GLOB_FI_ENABLECENTROID_FLAG){
        int height = GLOB_IMSIZE/GLOB_WIDTH;

        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);

        if (GLOB_FI_TIPTILTSERVO_FLAG){

            cv::Point2i DextraCentre(GLOB_FI_DEXTRA_CURRENT_CENTROID.x, GLOB_FI_DEXTRA_CURRENT_CENTROID.y);
            cv::Point2i SinistraCentre(GLOB_FI_SINISTRA_CURRENT_CENTROID.x, GLOB_FI_SINISTRA_CURRENT_CENTROID.y);

            auto DextraP = centroid_funcs::getCentroidWCOG(img, DextraCentre, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_INTERP_SIZE, GLOB_FI_CENTROID_GAIN);
            auto SinistraP = centroid_funcs::getCentroidWCOG(img, SinistraCentre, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_INTERP_SIZE, GLOB_FI_CENTROID_GAIN);
            
            pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
            GLOB_FI_DEXTRA_CURRENT_CENTROID.x = DextraP.x;
            GLOB_FI_DEXTRA_CURRENT_CENTROID.y = DextraP.y;
            GLOB_FI_SINISTRA_CURRENT_CENTROID.x = SinistraP.x;
            GLOB_FI_SINISTRA_CURRENT_CENTROID.x = SinistraP.y;

            GLOB_FI_DEXTRA_DIFF_POSITION.x = GLOB_FI_DEXTRA_TARGET_CENTROID.x - DextraP.x;
            GLOB_FI_DEXTRA_DIFF_POSITION.y = GLOB_FI_DEXTRA_TARGET_CENTROID.y - DextraP.y;
            GLOB_FI_SINISTRA_DIFF_POSITION.x = GLOB_FI_SINISTRA_TARGET_CENTROID.x - SinistraP.x;
            GLOB_FI_SINISTRA_DIFF_POSITION.y = GLOB_FI_SINISTRA_TARGET_CENTROID.y - SinistraP.y;
            pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
            
            std::string result = CA_SOCKET->send<std::string>("receiveTipTiltPos", GLOB_FI_DEXTRA_DIFF_POSITION, GLOB_FI_SINISTRA_DIFF_POSITION);
            cout << result << endl;

        } else {

            cv::Point2i DextraCentre(GLOB_FI_DEXTRA_TARGET_CENTROID.x, GLOB_FI_DEXTRA_TARGET_CENTROID.y);
            cv::Point2i SinistraCentre(GLOB_FI_SINISTRA_TARGET_CENTROID.x, GLOB_FI_SINISTRA_TARGET_CENTROID.y);

            auto DextraP = centroid_funcs::windowCentroidWCOG(img, GLOB_FI_INTERP_SIZE, GLOB_FI_GAUSS_RAD, DextraCentre, 
                                            GLOB_FI_WINDOW_SIZE, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_CENTROID_GAIN);
            auto SinistraP = centroid_funcs::windowCentroidWCOG(img, GLOB_FI_INTERP_SIZE, GLOB_FI_GAUSS_RAD, SinistraCentre, 
                                            GLOB_FI_WINDOW_SIZE, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_CENTROID_GAIN);

            pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
            GLOB_FI_DEXTRA_CURRENT_CENTROID.x = DextraP.x;
            GLOB_FI_DEXTRA_CURRENT_CENTROID.y = DextraP.y;
            GLOB_FI_SINISTRA_CURRENT_CENTROID.x = SinistraP.x;
            GLOB_FI_SINISTRA_CURRENT_CENTROID.x = SinistraP.y;

            GLOB_FI_DEXTRA_DIFF_POSITION.x = GLOB_FI_DEXTRA_TARGET_CENTROID.x - DextraP.x;
            GLOB_FI_DEXTRA_DIFF_POSITION.y = GLOB_FI_DEXTRA_TARGET_CENTROID.y - DextraP.y;
            GLOB_FI_SINISTRA_DIFF_POSITION.x = GLOB_FI_SINISTRA_TARGET_CENTROID.x - SinistraP.x;
            GLOB_FI_SINISTRA_DIFF_POSITION.y = GLOB_FI_SINISTRA_TARGET_CENTROID.y - SinistraP.y;

            if (GLOB_FI_SET_TARGET_FLAG){
                GLOB_FI_DEXTRA_TARGET_CENTROID.x = GLOB_FI_DEXTRA_CURRENT_CENTROID.x;
                GLOB_FI_DEXTRA_TARGET_CENTROID.y = GLOB_FI_DEXTRA_CURRENT_CENTROID.y;
                GLOB_FI_SINISTRA_TARGET_CENTROID.x = GLOB_FI_SINISTRA_CURRENT_CENTROID.x;
                GLOB_FI_SINISTRA_TARGET_CENTROID.y = GLOB_FI_SINISTRA_CURRENT_CENTROID.y;
                GLOB_FI_SET_TARGET_FLAG = 0;
                
            }
            pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        }
    }
    return 0;
}


// FLIR Camera Server
struct FiberInjection: FLIRCameraServer{

    FiberInjection() : FLIRCameraServer(FibreInjectionCallback){

        // Set up client parameters
        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string CA_port = config["FibreInjection"]["CA_port"].value_or("4100");
        std::string IP = config["FibreInjection"]["IP"].value_or("192.168.1.3");

        // Turn into a TCPString
        std::string CA_TCP = "tcp://" + IP + ":" + CA_port;
        
        CA_SOCKET = new commander::client::Socket(CA_TCP);

        GLOB_FI_DEXTRA_TARGET_CENTROID.x = config["FibreInjection"]["Dextra"]["target_x"].value_or(720.0);
        GLOB_FI_DEXTRA_TARGET_CENTROID.y = config["FibreInjection"]["Dextra"]["target_y"].value_or(420.0);
        
        GLOB_FI_SINISTRA_TARGET_CENTROID.x = config["FibreInjection"]["Sinistra"]["target_x"].value_or(720.0);
        GLOB_FI_SINISTRA_TARGET_CENTROID.y = config["FibreInjection"]["Sinistra"]["target_y"].value_or(420.0);

        GLOB_FI_DEXTRA_DIFF_POSITION.x = 0.0;
        GLOB_FI_DEXTRA_DIFF_POSITION.y = 0.0;

        GLOB_FI_SINISTRA_DIFF_POSITION.x = 0.0;
        GLOB_FI_SINISTRA_DIFF_POSITION.y = 0.0;

        GLOB_FI_INTERP_SIZE = config["FibreInjection"]["Centroid"]["interp_size"].value_or(7);
        GLOB_FI_GAUSS_RAD = config["FibreInjection"]["Centroid"]["gaussian_radius"].value_or(10);
        GLOB_FI_WINDOW_SIZE = config["FibreInjection"]["Centroid"]["gaussian_window_size"].value_or(50);
        GLOB_FI_CENTROID_GAIN = config["FibreInjection"]["Centroid"]["WCOG_gain"].value_or(1.0);

        double sigma = config["FibreInjection"]["Centroid"]["WCOG_sigma"].value_or(1.0);
        int img_type = config["FibreInjection"]["Centroid"]["img_type"].value_or(2);

        GLOB_FI_CENTROID_WEIGHTS = centroid_funcs::weightFunction(GLOB_FI_INTERP_SIZE, sigma, img_type);

    }

    ~FiberInjection(){
        delete CA_SOCKET;
    }

    centroid getDiffPosition(int index){
        centroid dpos;
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        if (index == 1){
            dpos = GLOB_FI_DEXTRA_DIFF_POSITION;
        } else if (index == 2){
            dpos = GLOB_FI_SINISTRA_DIFF_POSITION;
        } else {
            cout << "BAD INDEX" << endl;
            dpos.x = 0.0;
            dpos.y = 0.0;
        }
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return dpos;
    }

    centroid getTargetPosition(int index){
        centroid pos;
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        if (index == 1){
            pos = GLOB_FI_DEXTRA_TARGET_CENTROID;
        } else if (index == 2){
            pos = GLOB_FI_SINISTRA_TARGET_CENTROID;
        } else {
            cout << "BAD INDEX" << endl;
            pos.x = 0.0;
            pos.y = 0.0;
        }
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return pos;
    }

    centroid getCurrentPosition(int index){
        centroid pos;
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        if (index == 1){
            pos = GLOB_FI_DEXTRA_CURRENT_CENTROID;
        } else if (index == 2){
            pos = GLOB_FI_SINISTRA_CURRENT_CENTROID;
        } else {
            cout << "BAD INDEX" << endl;
            pos.x = 0.0;
            pos.y = 0.0;
        }
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return pos;
    }

    string enableCentroiding(int flag){
        string ret_msg = "Changing centroiding flag to: " + to_string(flag);
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        GLOB_FI_ENABLECENTROID_FLAG = flag;
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return ret_msg;
    }

    string enableTipTiltServo(int flag){
        string ret_msg = "Changing enable flag to: " + to_string(flag);
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        GLOB_FI_TIPTILTSERVO_FLAG = flag;
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return ret_msg;
    }

    string setTargetPosition(){
        string ret_msg;
        if (GLOB_FI_SET_TARGET_FLAG){
            ret_msg = "Target flag already set";
        } else if (GLOB_FI_TIPTILTSERVO_FLAG){
            ret_msg = "Can't set target when tip/tilt servo is running";
        } else {
            pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
            GLOB_FI_SET_TARGET_FLAG = 1;
            pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
            ret_msg = "Target flag set";
        }
        return ret_msg;
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
        .def("getparams", &FiberInjection::getparams, "Get all parameters")
        .def("get_diff_position", &FiberInjection::getDiffPosition, "Get differential position")
        .def("get_target_position", &FiberInjection::getTargetPosition, "Get target position")
        .def("set_target_position", &FiberInjection::setTargetPosition, "Set target position (when centroid running with corner cubes)")
        .def("get_current_position", &FiberInjection::getCurrentPosition, "Get current position")
        .def("enable_centroiding", &FiberInjection::enableCentroiding, "Enable centroiding")
        .def("enable_tiptiltservo", &FiberInjection::enableTipTiltServo, "Enable tip/tilt servo loop");
}
