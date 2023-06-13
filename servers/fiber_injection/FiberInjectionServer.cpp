#include <fstream>
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "FLIRcamServerFuncs.h"
#include "globals.h"
#include "toml.hpp"
#include "centroid.hpp"
#include <opencv2/opencv.hpp>

#include <chrono>
#include <ctime>

struct injection_centroids{
    centroid current_pos {0.0,0.0};
    centroid target_pos {0.0,0.0};
    centroid diff_pos {0.0,0.0};
    centroid centre {0.0,0.0};
};

struct centroid_settings{
    int interp_size;
    int gaussian_radius;
    int window_size;
    int gain;
    cv::Mat weights;
};

struct ROI{
    int width;
    int height;
    int offset_x;
    int offset_y;
};

pthread_mutex_t GLOB_FI_FLAG_LOCK;

commander::client::Socket* CA_SOCKET;

int GLOB_FI_ENABLECENTROID_FLAG;
int GLOB_FI_TIPTILTSERVO_FLAG;
int GLOB_FI_SET_TARGET_FLAG;

int GLOB_FI_PRINT_COUNTER = 0;

std::chrono::time_point<std::chrono::system_clock> GLOB_FI_PREVIOUS = std::chrono::system_clock::now();

injection_centroids GLOB_FI_DEXTRA_CENTROIDS;
injection_centroids GLOB_FI_SINISTRA_CENTROIDS;

centroid_settings GLOB_FI_COARSE_SETTINGS;
centroid_settings GLOB_FI_FINE_SETTINGS;

ROI GLOB_FI_COARSE_ROI;

double GLOB_FI_SERVO_GAIN;

using json = nlohmann::json;

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

int roundUp(int numToRound, int multiple) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound + multiple - 1) & -multiple;
}

int roundDown(int numToRound, int multiple) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound) & -multiple;
}

ROI calcROI(){
    ROI ret_ROI;
    pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
    injection_centroids dCentroid = GLOB_FI_DEXTRA_CENTROIDS;
    injection_centroids sCentroid = GLOB_FI_SINISTRA_CENTROIDS;
    centroid_settings c_fine = GLOB_FI_FINE_SETTINGS;
    pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);

    int width = abs(dCentroid.target_pos.x - sCentroid.target_pos.x) + c_fine.window_size;
    int height = abs(dCentroid.target_pos.y - sCentroid.target_pos.y) + c_fine.window_size;

    int xoffset, yoffset;

    if (dCentroid.target_pos.x < sCentroid.target_pos.x){
        xoffset = dCentroid.target_pos.x - c_fine.window_size/2;
    } else {
        xoffset = sCentroid.target_pos.x - c_fine.window_size/2;
    }
    if (dCentroid.target_pos.y < sCentroid.target_pos.x){
        yoffset = dCentroid.target_pos.y - c_fine.window_size/2;
    } else {
        yoffset = sCentroid.target_pos.y - c_fine.window_size/2;
    }

    ret_ROI.width = roundUp(width,4)+4;
    ret_ROI.height = roundUp(height,4)+4;
    ret_ROI.offset_x = roundDown(xoffset,4);
    ret_ROI.offset_y = roundDown(yoffset,4);

    return ret_ROI;
}


// Return 1 if error!
int FibreInjectionCallback (unsigned short* data){
    if (GLOB_FI_ENABLECENTROID_FLAG){
        int height = GLOB_IMSIZE/GLOB_WIDTH;

        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);
        
        centroid_settings temp_settings;
        cv::Point2i DextraCentre, SinistraCentre;
        
        cv::Point2i OffsetI = cv::Point2i(GLOB_CONFIG_PARAMS.offsetX, GLOB_CONFIG_PARAMS.offsetY);
        
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        if (GLOB_FI_TIPTILTSERVO_FLAG){  
            temp_settings = GLOB_FI_FINE_SETTINGS;
            DextraCentre = cv::Point2i(GLOB_FI_DEXTRA_CENTROIDS.target_pos.x, GLOB_FI_DEXTRA_CENTROIDS.target_pos.y) - OffsetI;
            SinistraCentre = cv::Point2i(GLOB_FI_SINISTRA_CENTROIDS.target_pos.x, GLOB_FI_SINISTRA_CENTROIDS.target_pos.y) - OffsetI;
        } else {
            temp_settings = GLOB_FI_COARSE_SETTINGS;
            DextraCentre = cv::Point2i(GLOB_FI_DEXTRA_CENTROIDS.centre.x, GLOB_FI_DEXTRA_CENTROIDS.centre.y) - OffsetI;
            SinistraCentre = cv::Point2i(GLOB_FI_SINISTRA_CENTROIDS.centre.x, GLOB_FI_SINISTRA_CENTROIDS.centre.y) - OffsetI;
        }
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        
        cv::Point2d OffsetD = cv::Point2d(GLOB_CONFIG_PARAMS.offsetX, GLOB_CONFIG_PARAMS.offsetY);
        auto DextraP = centroid_funcs::windowCentroidWCOG(img, temp_settings.interp_size, temp_settings.gaussian_radius, DextraCentre, 
                                        temp_settings.window_size, temp_settings.weights, temp_settings.gain) + OffsetD;
        auto SinistraP = centroid_funcs::windowCentroidWCOG(img, temp_settings.interp_size, temp_settings.gaussian_radius, SinistraCentre, 
                                        temp_settings.window_size, temp_settings.weights, temp_settings.gain) + OffsetD;

        centroid DexP, SinP;
        DexP.x = DextraP.x;
        DexP.y = DextraP.y;
        SinP.x = SinistraP.x;
        SinP.y = SinistraP.y;
        
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        GLOB_FI_DEXTRA_CENTROIDS.current_pos = DexP;
        GLOB_FI_SINISTRA_CENTROIDS.current_pos = SinP;
        GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x = GLOB_FI_DEXTRA_CENTROIDS.target_pos.x - DexP.x;
        GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y = GLOB_FI_DEXTRA_CENTROIDS.target_pos.y - DexP.y;
        GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x = GLOB_FI_SINISTRA_CENTROIDS.target_pos.x - SinP.x;
        GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y = GLOB_FI_SINISTRA_CENTROIDS.target_pos.y - SinP.y;
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);

        if (GLOB_FI_TIPTILTSERVO_FLAG){  
            string result = CA_SOCKET->send<string>("CA.receiveRelativeTipTiltPos", GLOB_FI_DEXTRA_CENTROIDS.diff_pos, GLOB_FI_SINISTRA_CENTROIDS.diff_pos, GLOB_FI_SERVO_GAIN);
            cout << result << endl;

        } else {
             if (GLOB_FI_SET_TARGET_FLAG){
                pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
                GLOB_FI_DEXTRA_CENTROIDS.target_pos = DexP;
                GLOB_FI_SINISTRA_CENTROIDS.target_pos = SinP;
                GLOB_FI_SET_TARGET_FLAG = 0;
                pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
            }
        }
        if (GLOB_FI_PRINT_COUNTER > 5){
            cout << "DcX: " <<  GLOB_FI_DEXTRA_CENTROIDS.current_pos.x << endl;
            cout << "DcY: " <<  GLOB_FI_DEXTRA_CENTROIDS.current_pos.y  << endl;
            cout << "ScX: " <<  GLOB_FI_SINISTRA_CENTROIDS.current_pos.x << endl;
            cout << "ScY: " << GLOB_FI_SINISTRA_CENTROIDS.current_pos.y << endl;         
            cout << "DtX: " <<  GLOB_FI_DEXTRA_CENTROIDS.target_pos.x << endl;
            cout << "DtY: " <<  GLOB_FI_DEXTRA_CENTROIDS.target_pos.y  << endl;
            cout << "StX: " <<  GLOB_FI_SINISTRA_CENTROIDS.target_pos.x << endl;
            cout << "StY: " << GLOB_FI_SINISTRA_CENTROIDS.target_pos.y << endl;                   
            cout << "DdX: " <<  GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x << endl;
            cout << "DdY: " <<  GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y  << endl;
            cout << "SdX: " <<  GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x << endl;
            cout << "SdY: " <<  GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y << endl;
            
            double Dextra_angle = 1.5708;
            double Sinistra_angle = 1.2217;
                        
            double Dx = cos(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x - sin(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y;
            double Dy = -sin(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x - cos(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y;
            double Sx = cos(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x  - sin(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y;
            double Sy = sin(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x  + cos(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y;
            
            cout << " DX: " << Dx << " DY: " << Dy << " SX: " << Sx << " SY: " << Sy << endl;
        }
    }
    std::chrono::time_point<std::chrono::system_clock> end;
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - GLOB_FI_PREVIOUS;
    GLOB_FI_PREVIOUS = end;
    if (GLOB_FI_PRINT_COUNTER > 5){
        GLOB_FI_PRINT_COUNTER = 0;
        cout << "FPS: " << 1/elapsed_seconds.count() << endl;
    }
    GLOB_FI_PRINT_COUNTER++;
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

        GLOB_FI_DEXTRA_CENTROIDS.target_pos.x = config["FibreInjection"]["Dextra"]["target_x"].value_or(720.0);
        GLOB_FI_DEXTRA_CENTROIDS.target_pos.y = config["FibreInjection"]["Dextra"]["target_y"].value_or(420.0);
        
        GLOB_FI_SINISTRA_CENTROIDS.target_pos.x = config["FibreInjection"]["Sinistra"]["target_x"].value_or(720.0);
        GLOB_FI_SINISTRA_CENTROIDS.target_pos.y = config["FibreInjection"]["Sinistra"]["target_y"].value_or(420.0);

        GLOB_FI_DEXTRA_CENTROIDS.centre.x = config["FibreInjection"]["Dextra"]["centre_x"].value_or(720.0);
        GLOB_FI_DEXTRA_CENTROIDS.centre.y = config["FibreInjection"]["Dextra"]["centre_y"].value_or(420.0);
        
        GLOB_FI_SINISTRA_CENTROIDS.centre.x = config["FibreInjection"]["Sinistra"]["centre_x"].value_or(720.0);
        GLOB_FI_SINISTRA_CENTROIDS.centre.y = config["FibreInjection"]["Sinistra"]["centre_y"].value_or(420.0);


        GLOB_FI_COARSE_SETTINGS.interp_size = config["FibreInjection"]["CoarseCentroid"]["interp_size"].value_or(9);
        GLOB_FI_COARSE_SETTINGS.gaussian_radius = config["FibreInjection"]["CoarseCentroid"]["gaussian_radius"].value_or(9);
        GLOB_FI_COARSE_SETTINGS.window_size = config["FibreInjection"]["CoarseCentroid"]["gaussian_window_size"].value_or(250);
        GLOB_FI_COARSE_SETTINGS.gain = config["FibreInjection"]["CoarseCentroid"]["WCOG_gain"].value_or(1);
        double sigma = config["FibreInjection"]["CoarseCentroid"]["WCOG_sigma"].value_or(4);
        GLOB_FI_COARSE_SETTINGS.weights = centroid_funcs::weightFunction(GLOB_FI_COARSE_SETTINGS.interp_size, sigma);

        GLOB_FI_FINE_SETTINGS.interp_size = config["FibreInjection"]["FineCentroid"]["interp_size"].value_or(9);
        GLOB_FI_FINE_SETTINGS.gaussian_radius = config["FibreInjection"]["FineCentroid"]["gaussian_radius"].value_or(9);
        GLOB_FI_FINE_SETTINGS.window_size = config["FibreInjection"]["FineCentroid"]["gaussian_window_size"].value_or(100);
        GLOB_FI_FINE_SETTINGS.gain = config["FibreInjection"]["FineCentroid"]["WCOG_gain"].value_or(1);
        sigma = config["FibreInjection"]["FineCentroid"]["WCOG_sigma"].value_or(4);
        GLOB_FI_FINE_SETTINGS.weights = centroid_funcs::weightFunction(GLOB_FI_FINE_SETTINGS.interp_size, sigma);

        GLOB_FI_COARSE_ROI.width = config["FLIRcamera"]["camera"]["width"].value_or(1000);
        GLOB_FI_COARSE_ROI.height = config["FLIRcamera"]["camera"]["height"].value_or(1000); 
        GLOB_FI_COARSE_ROI.offset_x = config["FLIRcamera"]["camera"]["offset_x"].value_or(0); 
        GLOB_FI_COARSE_ROI.offset_y = config["FLIRcamera"]["camera"]["offset_y"].value_or(0);

        GLOB_FI_SERVO_GAIN = config["FibreInjection"]["servo_gain"].value_or(0.1);

    }

    ~FiberInjection(){
        delete CA_SOCKET;
    }

    centroid getDiffPosition(int index){
        centroid dpos;
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        if (index == 1){
            dpos = GLOB_FI_DEXTRA_CENTROIDS.diff_pos;
        } else if (index == 2){
            dpos = GLOB_FI_SINISTRA_CENTROIDS.diff_pos;
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
            pos = GLOB_FI_DEXTRA_CENTROIDS.target_pos;
        } else if (index == 2){
            pos = GLOB_FI_SINISTRA_CENTROIDS.target_pos;
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
            pos = GLOB_FI_DEXTRA_CENTROIDS.current_pos;
        } else if (index == 2){
            pos = GLOB_FI_SINISTRA_CENTROIDS.current_pos;
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

    void change_ROI(ROI roi_settings){
        string ret_msg;
        ret_msg = this->reconfigure_offsetX(0);    
        cout << ret_msg << endl;
        ret_msg = this->reconfigure_offsetY(0);    
        cout << ret_msg << endl;
        ret_msg = this->reconfigure_width(roi_settings.width);    
        cout << ret_msg << endl;
        ret_msg = this->reconfigure_height(roi_settings.height);    
        cout << ret_msg << endl;
        ret_msg = this->reconfigure_offsetX(roi_settings.offset_x);    
        cout << ret_msg << endl;
        ret_msg = this->reconfigure_offsetY(roi_settings.offset_y);    
        cout << ret_msg << endl;
        return;
    }

    string enableTipTiltServo(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                if (flag == GLOB_FI_TIPTILTSERVO_FLAG){
                    ret_msg = "Flag is already set";
                } else{
                    // First, stop the camera if running
                    ret_msg = this->stopcam();
                    while (GLOB_RUNNING == 1){
                        usleep(1000);
                    }
                    cout << ret_msg << endl;

                    if (flag == 1){
                        // Reconfigure to fine centroiding
                        ROI fineROI = calcROI();
                        this->change_ROI(fineROI);    
                    } else {
                        // Reconfigure to coarse centroiding
                        this->change_ROI(GLOB_FI_COARSE_ROI);    
                    }

                    while (GLOB_RECONFIGURE == 1){
                        usleep(1000);
                    }
                    pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
                    GLOB_FI_TIPTILTSERVO_FLAG = flag;
                    pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
                    ret_msg = this->startcam(GLOB_NUMFRAMES,GLOB_COADD);
                    cout << ret_msg << endl;
                    
                    ret_msg = "Changing tip/tilt enable flag to: " + to_string(flag);
                } 
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }        
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
