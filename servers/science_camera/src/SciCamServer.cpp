
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include "QHYcamServerFuncs.h"

commander::client::Socket* CA_SOCKET;

int GLOB_SC_DARK_FLAG;
int GLOB_SC_FLUX_FLAG;
int GLOB_SC_SCAN_FLAG;
int GLOB_SC_STAGE = 0;
double GLOB_SC_V2SNR_THRESHOLD;

pthread_mutex_t GLOB_SC_FLAG_LOCK;

// Return 1 if error!
int GroupDelayCallback (unsigned short* data){
    int ret_val;
    if (GLOB_SC_DARK_FLAG){
        ret_val = measureDark(data);
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_STAGE = 1;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
    } else if (GLOB_SC_FLUX_FLAG == 1){
        if (GLOB_SC_STAGE == 1){
            ret_val = addToFlux(data,1);
            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
            GLOB_SC_STAGE = 2;
            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        } else {
            cout << "HAVE NOT SAVED DARKS YET" << endl; 
        }
    } else if (GLOB_SC_FLUX_FLAG == 2){
        if (GLOB_SC_STAGE == 2){
            ret_val = addToFlux(data,2);
            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
            GLOB_SC_STAGE = 3;
            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        } else {
            cout << "HAVE NOT SAVED FLUX 1 YET" << endl; 
        }
    } else {
        if (GLOB_SC_STAGE == 4){
            retVal = calcGroupDelay(data);

            if (GLOB_SC_SCAN_FLAG){
                cout << GLOB_SC_V2SNR << endl;
                if (GLOB_SC_V2SNR > GLOB_SC_V2SNR_THRESHOLD){
                    pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                    GLOB_SC_SCAN_FLAG = 0;
                    pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                }
            } else{
                cout << GLOB_SC_GD << endl;
                std::string result = CA_SOCKET->send<std::string>("receiveGroupDelay", GLOB_SC_GD)
                cout << result << endl;
            }

        } else {
            cout << "HAVE NOT CREATED P2VM MATRIX YET" << endl;   
        }
    }

    return 0;
}


// FLIR Camera Server
struct SciCam: QHYCameraServer{

    SciCam() : QHYCameraServer(GroupDelayCallback){

        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string CA_port = config["ScienceCamera"]["CA_port"].value_or("4100");
        std::string IP = config["ScienceCamera"]["IP"].value_or("192.168.1.3");

        // Turn into a TCPString
        std::string CA_TCP = "tcp://" + IP + ":" + CA_port;
        
        CA_SOCKET = new commander::client::Socket(CA_TCP);

        int xref = config["ScienceCamera"]["xref"].value_or(500);
        int yref = config["ScienceCamera"]["yref"].value_or(500);

        setPixelPositions(xref,yref);

        int numDelays = config["ScienceCamera"]["numDelays"].value_or(1000);
        double delaySize = config["ScienceCamera"]["delaySize"].value_or(1);    

        GLOB_SC_V2SNR_THRESHOLD = config["ScienceCamera"]["SNRThreshold"].value_or(10);

        calcTrialDelayMat(numDelays,delaySize);

    }

    ~SciCam(){
        delete CA_SOCKET;
    }

    string enableDarks(int flag){
        string ret_msg = "Changing Dark flag to: " + to_string(flag);
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_DARK_FLAG = flag;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        return ret_msg;
    }

    string enableFluxes(int flag){
        string ret_msg;
        if (GLOB_SC_STAGE > 0){
            string status;
            if (flag == 0){
                status = "off";
            } else if (flag == 1){
                status = "Dextra";
            } else if (flag == 2){
                status = "Sinistra";
            } else {
                status = "UNKNOWN INDEX";
            }
            ret_msg = "Changing Flux flag to: " + status;
            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
            GLOB_SC_FLUX_FLAG = flag;
            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        } else {
            ret_msg = "Please save Darks first";
        }
        return ret_msg;
    }

    string calcP2VM(){
        string ret_msg;
        if (GLOB_SC_STAGE > 2){
            ret_msg = "Setting up P2VM Matrix";
            calcP2VMMat();
        } else if (GLOB_SC_STAGE == 0){
            ret_msg = "Please save Darks first";
        } else if (GLOB_SC_STAGE == 1){
            ret_msg = "Please save Flux Dextra first";
        } else if (GLOB_SC_STAGE == 2){
            ret_msg = "Please save Flux Sinistra first";
        }
        return ret_msg;
    }

    string getV2SNRestimate(){
        double a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_V2SNR;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        string ret_msg = "V2 SNR Estimate is " + a;
        return ret_msg;
    }

    string getGDestimate(){
        double a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_GD;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        string ret_msg = "Group Delay Estimate is " + a;
        return ret_msg;
    }

    ?? getV2array(){

    }

    ??? getGDarray(){

    }

    string FRINGESCANNING(){
        
    }

};

// Register as commander server
// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<SciCam>("SC")
        // To insterface a class method, you can use the `def` method.
        .def("status", &SciCam::status, "Camera Status")
        .def("connect", &SciCam::connectcam, "Connect the camera")
        .def("disconnect", &SciCam::disconnectcam, "Disconnect the camera")
        .def("start", &SciCam::startcam, "Start exposures")
        .def("stop", &SciCam::stopcam, "Stop exposures")
        .def("getlatestfilename", &SciCam::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &SciCam::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &SciCam::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &SciCam::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &SciCam::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &SciCam::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &SciCam::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &SciCam::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &SciCam::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &SciCam::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &SciCam::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &SciCam::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &SciCam::getparams, "Get all parameters")
        .def("getgroupdelay", &SciCam::getGroupDelay, "Get current group delay estimate")
        .def("getgroupdelayarr", &SciCam::getGroupDelayArr, "Get group delay array amplitudes")
        .def("calibrateP2VM", &SciCam::calibrateP2VM, "Get current group delay estimate")
        .def("setwavescale", &SciCam::setWaveScale, "Get current group delay estimate")
        .def("calctrialdelays", &SciCam::calcTrialDelays, "Get current group delay estimate")
        .def("setpixelpos", &SciCam::setPixelPos, "Get current group delay estimate")
        .def("savecalibration", &SciCam::saveCal, "Save calibration data to file")
        .def("readcalibration", &SciCam::readCal, "Read calibration data from file");

}
