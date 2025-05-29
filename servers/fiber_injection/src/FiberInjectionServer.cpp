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

/*
Struct to hold information on relevant centroids:
    current_pos - current measured centroid
    target_pos - centroid to aim for
    diff_pos - difference between current and target
    centre - centre position of the FOV window
*/ 

struct injection_centroids{
    centroid current_pos {0.0,0.0};
    centroid target_pos {0.0,0.0};
    centroid diff_pos {0.0,0.0};
    centroid centre {0.0,0.0};
};


/*
Centroiding settings for the various modes:
    interp_size - number of pixels to interpolate centroid over
    gaussian_radius - radius of the gaussian blur
    window_size - size of the window to gaussian blur over
    gain - gain of WCOG centroid technique
    weights - weights of the WCOG centroid technique
*/
struct centroid_settings{
    int interp_size;
    int gaussian_radius;
    int window_size;
    int gain;
    cv::Mat weights;
};

/*
Struct to hold a region of interest
*/
struct ROI{
    int width;
    int height;
    int offset_x;
    int offset_y;
};

pthread_mutex_t GLOB_FI_FLAG_LOCK;

commander::client::Socket* CA_SOCKET;

int GLOB_FI_ENABLECENTROID_FLAG; // Flag to enable/disable centroiding
int GLOB_FI_TIPTILTSERVO_FLAG; // Flag to do fine tip/tilt servoing. 0 for none, 1 for Dextra, 2 for Sinistra, 3 for both.
int GLOB_FI_SET_TARGET_FLAG; // Set a new target position flag

int GLOB_FI_PRINT_COUNTER = 0;

std::chrono::time_point<std::chrono::system_clock> GLOB_FI_PREVIOUS = std::chrono::system_clock::now();

// Centroid structs for the two deputies
injection_centroids GLOB_FI_DEXTRA_CENTROIDS;
injection_centroids GLOB_FI_SINISTRA_CENTROIDS;

// Two different modes: fine and coarse. Fine is the Tip/Tilt servo mode, wheras coarse is designed
// to only be corrected by the deputy plate solver
centroid_settings GLOB_FI_COARSE_SETTINGS;
centroid_settings GLOB_FI_FINE_SETTINGS;

ROI GLOB_FI_COARSE_ROI;

double GLOB_FI_SERVO_GAIN;

using json = nlohmann::json;

// Centroid/JSON serialiser
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
Function to round up a number to the nearest multiple of another
Inputs:
    numToRound - number to round
    multiple - round numToRound up to the nearest multiple of this number
Outputs:
    rounded number
*/
int roundUp(int numToRound, int multiple) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound + multiple - 1) & -multiple;
}

/*
Function to round down a number to the nearest multiple of another
Inputs:
    numToRound - number to round
    multiple - round numToRound down to the nearest multiple of this number
Outputs:
    rounded number
*/
int roundDown(int numToRound, int multiple) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound) & -multiple;
}

/*
Calculate the ROI for the fine centroiding (for the tip/tilt servo)
Designed such that it will have both Sinistra and Dextra's spots in frame with the full motion of
the tip/tilt piezos.
Returns the ROI needed for the tip/tilt servo to run at ~200Hz
*/
ROI calcROI(){
    ROI ret_ROI;

    // Get centroid information
    pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
    injection_centroids dCentroid = GLOB_FI_DEXTRA_CENTROIDS;
    injection_centroids sCentroid = GLOB_FI_SINISTRA_CENTROIDS;
    centroid_settings c_fine = GLOB_FI_FINE_SETTINGS;
    pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);

    // Set the width to include both target spots, and additional buffer according to the window size
    int width = abs(dCentroid.target_pos.x - sCentroid.target_pos.x) + c_fine.window_size;
    int height = abs(dCentroid.target_pos.y - sCentroid.target_pos.y) + c_fine.window_size;

    int xoffset, yoffset;

    // Calculate offsets
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

    // Round for FLIR reasons
    ret_ROI.width = roundUp(width,4)+4;
    ret_ROI.height = roundUp(height,4)+4;
    ret_ROI.offset_x = roundDown(xoffset,4);
    ret_ROI.offset_y = roundDown(yoffset,4);

    return ret_ROI;
}

// Readout the centroid positions every n lines. Higher number means less readout
int Readout_Speed = 10;

/*
Callback function to do do centroiding!
Inputs:
    data - array of the raw camera data
Output:
    return 1 if error
*/
int FibreInjectionCallback (unsigned short* data){
    // Check if we are centroiding at all
    if (GLOB_FI_ENABLECENTROID_FLAG){
        int height = GLOB_IMSIZE/GLOB_WIDTH;

        cv::Mat img (height,GLOB_WIDTH,CV_16U,data);
        
        centroid_settings temp_settings;
        cv::Point2i DextraCentre, SinistraCentre;
        
        cv::Point2i OffsetI = cv::Point2i(GLOB_CONFIG_PARAMS.offsetX, GLOB_CONFIG_PARAMS.offsetY);
        
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        // If the tip/tilt servo is on, we are using the small ROI
        if (GLOB_FI_TIPTILTSERVO_FLAG > 0){  
            temp_settings = GLOB_FI_FINE_SETTINGS;
            // The centre should be the target positions!
            DextraCentre = cv::Point2i(GLOB_FI_DEXTRA_CENTROIDS.target_pos.x, GLOB_FI_DEXTRA_CENTROIDS.target_pos.y) - OffsetI;
            SinistraCentre = cv::Point2i(GLOB_FI_SINISTRA_CENTROIDS.target_pos.x, GLOB_FI_SINISTRA_CENTROIDS.target_pos.y) - OffsetI;
        } else { // Otherwise, use the larger ROI
            temp_settings = GLOB_FI_COARSE_SETTINGS;
            // The centre should be the defined FOV positions!
            DextraCentre = cv::Point2i(GLOB_FI_DEXTRA_CENTROIDS.centre.x, GLOB_FI_DEXTRA_CENTROIDS.centre.y) - OffsetI;
            SinistraCentre = cv::Point2i(GLOB_FI_SINISTRA_CENTROIDS.centre.x, GLOB_FI_SINISTRA_CENTROIDS.centre.y) - OffsetI;
        }
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        
        // Run the centroiding algorithm!
        cv::Point2d OffsetD = cv::Point2d(GLOB_CONFIG_PARAMS.offsetX, GLOB_CONFIG_PARAMS.offsetY);
        auto DextraP = centroid_funcs::windowCentroidWCOG(img, temp_settings.interp_size, temp_settings.gaussian_radius, DextraCentre, 
                                        temp_settings.window_size, temp_settings.weights, temp_settings.gain) + OffsetD;
        auto SinistraP = centroid_funcs::windowCentroidWCOG(img, temp_settings.interp_size, temp_settings.gaussian_radius, SinistraCentre, 
                                        temp_settings.window_size, temp_settings.weights, temp_settings.gain) + OffsetD;

        // Save and update the values
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

        // If flag is 3, send both Dextra and Sinistra differential positions
        if (GLOB_FI_TIPTILTSERVO_FLAG == 3){
            string result = CA_SOCKET->send<string>("CA.receiveRelativeTipTiltPos", GLOB_FI_DEXTRA_CENTROIDS.diff_pos, GLOB_FI_SINISTRA_CENTROIDS.diff_pos, GLOB_FI_SERVO_GAIN);
        } else if (GLOB_FI_TIPTILTSERVO_FLAG == 1){ // Otherwise just do Dextra
            centroid zero_pos {0.0,0.0};
            string result = CA_SOCKET->send<string>("CA.receiveRelativeTipTiltPos", GLOB_FI_DEXTRA_CENTROIDS.diff_pos, zero_pos, GLOB_FI_SERVO_GAIN);
        } else if (GLOB_FI_TIPTILTSERVO_FLAG == 2){ // Otherwise just do Sinistra
            centroid zero_pos {0.0,0.0};
            string result = CA_SOCKET->send<string>("CA.receiveRelativeTipTiltPos", zero_pos, GLOB_FI_SINISTRA_CENTROIDS.diff_pos, GLOB_FI_SERVO_GAIN);
        } else if (GLOB_FI_SET_TARGET_FLAG){ // Set a new target position!
            pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
            GLOB_FI_DEXTRA_CENTROIDS.target_pos = DexP;
            GLOB_FI_SINISTRA_CENTROIDS.target_pos = SinP;
            GLOB_FI_SET_TARGET_FLAG = 0;
            pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        }
        // Print every "Readout_Speed" iterations
        if (GLOB_FI_PRINT_COUNTER > Readout_Speed){
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
             
            // DEBUGGING LINES //
            //double Dextra_angle = 1.5708;
            //double Sinistra_angle = 1.2217;
                        
            //double Dx = cos(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x - sin(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y;
            //double Dy = -sin(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.x - cos(Dextra_angle)*GLOB_FI_DEXTRA_CENTROIDS.diff_pos.y;
            //double Sx = cos(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x  - sin(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y;
            //double Sy = sin(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.x  + cos(Sinistra_angle)*GLOB_FI_SINISTRA_CENTROIDS.diff_pos.y;
            
            //cout << " DX: " << Dx << " DY: " << Dy << " SX: " << Sx << " SY: " << Sy << endl;
        }
    }

    // Time the loop!
    std::chrono::time_point<std::chrono::system_clock> end;
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - GLOB_FI_PREVIOUS;
    GLOB_FI_PREVIOUS = end;
    if (GLOB_FI_PRINT_COUNTER > Readout_Speed){
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
        std::string IP = config["IP"].value_or("192.168.1.3");

        // Turn into a TCPString
        std::string CA_TCP = "tcp://" + IP + ":" + CA_port;
        
        CA_SOCKET = new commander::client::Socket(CA_TCP);

        // Set up initial positions
        GLOB_FI_DEXTRA_CENTROIDS.target_pos.x = config["FibreInjection"]["Dextra"]["target_x"].value_or(720.0);
        GLOB_FI_DEXTRA_CENTROIDS.target_pos.y = config["FibreInjection"]["Dextra"]["target_y"].value_or(420.0);
        
        GLOB_FI_SINISTRA_CENTROIDS.target_pos.x = config["FibreInjection"]["Sinistra"]["target_x"].value_or(720.0);
        GLOB_FI_SINISTRA_CENTROIDS.target_pos.y = config["FibreInjection"]["Sinistra"]["target_y"].value_or(420.0);

        GLOB_FI_DEXTRA_CENTROIDS.centre.x = config["FibreInjection"]["Dextra"]["centre_x"].value_or(720.0);
        GLOB_FI_DEXTRA_CENTROIDS.centre.y = config["FibreInjection"]["Dextra"]["centre_y"].value_or(420.0);
        
        GLOB_FI_SINISTRA_CENTROIDS.centre.x = config["FibreInjection"]["Sinistra"]["centre_x"].value_or(720.0);
        GLOB_FI_SINISTRA_CENTROIDS.centre.y = config["FibreInjection"]["Sinistra"]["centre_y"].value_or(420.0);

        //Set up centroiding parameters
        GLOB_FI_COARSE_SETTINGS.interp_size = config["FibreInjection"]["CoarseCentroid"]["interp_size"].value_or(9);
        GLOB_FI_COARSE_SETTINGS.gaussian_radius = config["FibreInjection"]["CoarseCentroid"]["gaussian_radius"].value_or(9);
        GLOB_FI_COARSE_SETTINGS.window_size = config["FibreInjection"]["CoarseCentroid"]["window_size"].value_or(250);
        GLOB_FI_COARSE_SETTINGS.gain = config["FibreInjection"]["CoarseCentroid"]["WCOG_gain"].value_or(1);
        double sigma = config["FibreInjection"]["CoarseCentroid"]["WCOG_sigma"].value_or(4);
        GLOB_FI_COARSE_SETTINGS.weights = centroid_funcs::weightFunction(GLOB_FI_COARSE_SETTINGS.interp_size, sigma);

        GLOB_FI_FINE_SETTINGS.interp_size = config["FibreInjection"]["FineCentroid"]["interp_size"].value_or(9);
        GLOB_FI_FINE_SETTINGS.gaussian_radius = config["FibreInjection"]["FineCentroid"]["gaussian_radius"].value_or(9);
        GLOB_FI_FINE_SETTINGS.window_size = config["FibreInjection"]["FineCentroid"]["window_size"].value_or(100);
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

    /*
    Function to get the differential position of one of the deputies
    Inputs:
        index - 1 for Dextra, 2 for Sinistra
    Output:
        returns differential centroid
    */
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

    /*
    Function to get the target position of one of the deputies
    Inputs:
        index - 1 for Dextra, 2 for Sinistra
    Output:
        returns target centroid
    */
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

    /*
    Function to get the current position of one of the deputies
    Inputs:
        index - 1 for Dextra, 2 for Sinistra
    Output:
        returns current centroid
    */
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

    /*
    Function enable centroiding (i.e the coarse version)
    Inputs:
        flag - 0 for off, 1 for on
    Output:
        returns status message
    */
    string enableCentroiding(int flag){
        string ret_msg = "Changing centroiding flag to: " + to_string(flag);
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        GLOB_FI_ENABLECENTROID_FLAG = flag;
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        return ret_msg;
    }

    /*
    Changes the Camera ROI
    Inputs:
        roi_settings - the ROI settings to change the camera to
    */
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

    /*
    Function to enable the fine tip/tilt servo loop.
    Inputs:
        flag - 0 for off, 1 for on
    Outputs:
        returns status message
    */
    string enableTipTiltServo(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                if (flag == GLOB_FI_TIPTILTSERVO_FLAG){
                    ret_msg = "Flag is already set";
                } else if (flag > 0 && GLOB_FI_TIPTILTSERVO_FLAG > 0){
                    pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
                    GLOB_FI_TIPTILTSERVO_FLAG = flag;
                    pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
                    ret_msg = "Changing tip/tilt enable flag to: " + to_string(flag);
                } else {
                    // First, stop the camera if running
                    ret_msg = this->stopcam();
                    cout << ret_msg << endl;

                    if (flag > 0){
                        // Reconfigure to fine centroiding
                        ROI fineROI = calcROI();
                        this->change_ROI(fineROI);    
                    } else {
                        // Reconfigure to coarse centroiding
                        this->change_ROI(GLOB_FI_COARSE_ROI);    
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

    /*
    Function to set a new target position
    */
    string setTargetPosition(){
        string ret_msg;
        if (GLOB_FI_SET_TARGET_FLAG){
            ret_msg = "Target flag already set";
        } else if (GLOB_FI_TIPTILTSERVO_FLAG > 0){
            ret_msg = "Can't set target when tip/tilt servo is running";
        } else {
            pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
            GLOB_FI_SET_TARGET_FLAG = 1;
            pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
            ret_msg = "Target flag set";
        }
        return ret_msg;
    }
    
    /*
    Function to set a new servo gain on the fly
    Inputs:
        gain - servo gain
    Outputs:
        returns status message
    */
    string setGain(double gain){
        string ret_msg;
        pthread_mutex_lock(&GLOB_FI_FLAG_LOCK);
        GLOB_FI_SERVO_GAIN = gain;
        pthread_mutex_unlock(&GLOB_FI_FLAG_LOCK);
        ret_msg = "Set servo gain to " + to_string(gain);
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
        .def("start", &FiberInjection::startcam, "Start exposures [number of frames, coadd flag]")
        .def("stop", &FiberInjection::stopcam, "Stop exposures")
        .def("getlatestfilename", &FiberInjection::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FiberInjection::getlatestimage, "Get the latest image data [compression parameter, binning flag]")
        .def("reconfigure_all", &FiberInjection::reconfigure_all, "Reconfigure all parameters [configuration struct as a json]")
        .def("reconfigure_gain", &FiberInjection::reconfigure_gain, "Reconfigure the gain [gain]")
        .def("reconfigure_exptime", &FiberInjection::reconfigure_exptime, "Reconfigure the exposure time [exptime in us]")
        .def("reconfigure_width", &FiberInjection::reconfigure_width, "Reconfigure the width [width in px]")
        .def("reconfigure_height", &FiberInjection::reconfigure_height, "Reconfigure the height [height in px]")
        .def("reconfigure_offsetX", &FiberInjection::reconfigure_offsetX, "Reconfigure the X offset [xoffset in px]")
        .def("reconfigure_offsetY", &FiberInjection::reconfigure_offsetY, "Reconfigure the Y offset [yoffset in px]")
        .def("reconfigure_blacklevel", &FiberInjection::reconfigure_blacklevel, "Reconfigure the black level [black_level]")
        .def("reconfigure_buffersize", &FiberInjection::reconfigure_buffersize, "Reconfigure the buffer size [buffer size in frames]")
        .def("reconfigure_savedir", &FiberInjection::reconfigure_savedir, "Reconfigure the save directory [save directory as a string]")
        .def("getparams", &FiberInjection::getparams, "Get all parameters")
        .def("resetUSBPort", &FiberInjection::resetUSBPort, "Reset the USB port on the HUB [string HUB name, string port number]")
        .def("get_diff_position", &FiberInjection::getDiffPosition, "Get differential position [index (1 for Dextra, 2 for Sinistra)]")
        .def("get_target_position", &FiberInjection::getTargetPosition, "Get target position  [index (1 for Dextra, 2 for Sinistra)]")
        .def("set_target_position", &FiberInjection::setTargetPosition, "Set target position (when centroid running with corner cubes)")
        .def("set_gain", &FiberInjection::setGain, "Set servo gain [gain]")
        .def("get_current_position", &FiberInjection::getCurrentPosition, "Get current position [index (1 for Dextra, 2 for Sinistra)]")
        .def("enable_centroiding", &FiberInjection::enableCentroiding, "Enable centroiding [flag]")
        .def("enable_tiptiltservo", &FiberInjection::enableTipTiltServo, "Enable tip/tilt servo loop [flag]");
}
