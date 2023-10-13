#include <fstream>
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <commander/client/socket.h>
#include "toml.hpp"
#include "QHYcamServerFuncs.h"
#include "globals.h"
#include "setup.hpp"
#include "group_delay.hpp"
#include <Eigen/Dense>

#include <chrono>
#include <ctime>

using json = nlohmann::json;

struct coord{
    double RA; //current
    double DEC; //voltage
};

// Serialiser to convert coord struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<coord> {
        static void to_json(json& j, const coord& c) {
            j = json{{"RA", c.RA}, {"DEC", c.DEC}};
        }

        static void from_json(const json& j, coord& c) {
            j.at("RA").get_to(c.RA);
            j.at("DEC").get_to(c.DEC);
        }
    };
}

commander::client::Socket* CA_SOCKET;
commander::client::Socket* TS_SOCKET;

int GLOB_SC_DARK_FLAG=0;
int GLOB_SC_FLUX_FLAG=0;
int GLOB_SC_FOREGROUND_FLAG=0;
int GLOB_SC_SCAN_FLAG=0;
int GLOB_SC_SERVO_FLAG =0;
int GLOB_SC_NEXT_SCAN_FLAG=0;
int GLOB_SC_STAGE = 0;
int GLOB_SC_TRACK_PERIOD;
int GLOB_SC_SCAN_PERIOD;
double GLOB_SC_V2SNR_THRESHOLD;
double GLOB_SC_GAIN;
int GLOB_SC_PRINT_COUNTER = 0;

int GLOB_SC_REACQ_FLAG = 0;
int GLOB_SC_REACQ_STAGE = 0;
int GLOB_SC_REACQ_CUR_STEP;
int GLOB_SC_REACQ_PRE_STEP = 0;
int GLOB_SC_REACQ_STEPSIZE;
double GLOB_SC_REACQ_THRESHOLD;

std::chrono::time_point<std::chrono::system_clock> GLOB_SC_PREVIOUS = std::chrono::system_clock::now();
// Return 1 if error!
int GroupDelayCallback (unsigned short* data){
    int ret_val;
    if (GLOB_SC_NEXT_SCAN_FLAG){
        //ret_val = fringeScan(data);
        //pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        //GLOB_SC_NEXT_SCAN_FLAG = 0;
        //pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        ret_val = fringeScan2(data);
    } else if (GLOB_SC_DARK_FLAG){
        ret_val = measureDark(data);
        if (GLOB_SC_STAGE == 0){
            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
            GLOB_SC_STAGE = 1;
            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        }
    } else if (GLOB_SC_FLUX_FLAG == 1){
        if (GLOB_SC_STAGE > 0){
            ret_val = addToFlux(data,1);
            if (GLOB_SC_STAGE == 1){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_STAGE = 2;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }
        } else {
            cout << "HAVE NOT SAVED DARKS YET" << endl;
            return 1;
        }
    } else if (GLOB_SC_FLUX_FLAG == 2){
        if (GLOB_SC_STAGE > 1){
            ret_val = addToFlux(data,2);
            if (GLOB_SC_STAGE == 2){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_STAGE = 3;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }
        } else {
            cout << "HAVE NOT SAVED FLUX 1 YET" << endl; 
            return 1;
        }
    } else if (GLOB_SC_STAGE == 3){
            cout << "HAVE NOT CREATED P2VM MATRIX YET" << endl; 
            return 1;  
    } else if (GLOB_SC_FOREGROUND_FLAG){
        if (GLOB_SC_STAGE > 3){
            ret_val = calcForeground(data);
            if (GLOB_SC_STAGE == 4){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_STAGE = 5;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }
        } else {
            cout << "HAVE NOT CREATED P2VM MATRIX YET" << endl; 
            return 1;
        }
    }  else if (GLOB_SC_STAGE == 5){
        ret_val = calcGroupDelay(data);
        if (GLOB_SC_PRINT_COUNTER > 20){
            cout << "GD: " << GLOB_SC_GD << endl;
            cout << "V2SNR: " << GLOB_SC_V2SNR << endl;
        }
        if (GLOB_SC_SCAN_FLAG){
            if (GLOB_SC_V2SNR > GLOB_SC_V2SNR_THRESHOLD){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_SCAN_FLAG = 0;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                // SEND STOP COMMAND and SET TO FAST FREQUENCY
                std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", 0, 1000);
                cout << "FOUND FRINGES" << endl;
                return 1;
            }
        } else if (GLOB_SC_SERVO_FLAG){
            if (GLOB_SC_V2SNR > GLOB_SC_REACQ_THRESHOLD){
                /////////////////// REACQUISITION /////////////////////////
                if (GLOB_SC_REACQ_FLAG){
                    if (GLOB_SC_V2SNR > GLOB_SC_V2SNR_THRESHOLD){
                        std::cout << "Ending Reacq" << std::endl;
                        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                        GLOB_SC_REACQ_FLAG = 0;
                        GLOB_SC_REACQ_STAGE = 0;
                        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                    } else {
                        GLOB_SC_REACQ_CUR_STEP = CA_SOCKET->send<int32_t>("CA.SDCpos"); // Get position
                        int32_t dest_step = GLOB_SC_REACQ_PRE_STEP + (2*(GLOB_SC_REACQ_STAGE%2)-1)*(GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE;
                        std::cout << dest_step << std::endl;
                        if (abs(GLOB_SC_REACQ_CUR_STEP - dest_step) < 10){
                            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                            GLOB_SC_REACQ_STAGE += 1;
                            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                            if (GLOB_SC_REACQ_STAGE%2 == 0){
                                // Move N steps forward
                                int32_t num_steps = static_cast<int32_t>((GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE);
                                std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, 100);
                            } else {
                                // Scan N steps backwards
                                int32_t num_steps = static_cast<int32_t>(-(GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE);
                                std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, GLOB_SC_SCAN_PERIOD);                     
                            }
                            pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                            GLOB_SC_REACQ_PRE_STEP = GLOB_SC_REACQ_CUR_STEP;
                            pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                        }
                        
                    }
                 /////////////////// END REACQUISITION /////////////////////////
                } else {  
                    
                    //int32_t num_steps = static_cast<int32_t>(GLOB_SC_GAIN*GLOB_SC_GD*50); //
                    double sign = copysign(1.0, GLOB_SC_GD);
                    int32_t num_steps = static_cast<int32_t>(sign*500);
                    uint16_t period = static_cast<int32_t>(abs(1000*GLOB_SC_GAIN/(GLOB_SC_GD*50)));
                    if (period < 100){
                        period = 100;
                    }
                    GLOB_SC_REACQ_CUR_STEP = CA_SOCKET->send<int32_t>("CA.SDCpos");
                    std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, period);
                    std::ofstream myfile;
                    myfile.open ("GD_servo_data.txt",std::ios_base::app);
                    myfile << GLOB_SC_GD << "," << GLOB_SC_REACQ_CUR_STEP << "\n";
                    myfile.close();
                }
                
            } else if (GLOB_SC_V2SNR <= GLOB_SC_REACQ_THRESHOLD){
                /////////////////// REACQUISITION /////////////////////////
                if (GLOB_SC_REACQ_FLAG){
                    GLOB_SC_REACQ_CUR_STEP = CA_SOCKET->send<int32_t>("CA.SDCpos"); // Get position
                    int32_t dest_step = GLOB_SC_REACQ_PRE_STEP + (2*(GLOB_SC_REACQ_STAGE%2)-1)*(GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE;
                    std::cout << dest_step << std::endl;
                    if (abs(GLOB_SC_REACQ_CUR_STEP - dest_step) < 10){
                        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                        GLOB_SC_REACQ_STAGE += 1;
                        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                        if (GLOB_SC_REACQ_STAGE%2 == 0){
                            // Move N steps forward
                            int32_t num_steps = static_cast<int32_t>((GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE);
                            std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, 100);
                        } else {
                            // Scan N steps backwards
                            int32_t num_steps = static_cast<int32_t>(-(GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE);
                            std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, GLOB_SC_SCAN_PERIOD);              
                        }
                        GLOB_SC_REACQ_PRE_STEP = GLOB_SC_REACQ_CUR_STEP;
                    }

                } else{
                    std::cout << "Starting Reacq" << std::endl;   
                    GLOB_SC_REACQ_CUR_STEP = CA_SOCKET->send<int32_t>("CA.SDCpos"); // Get position
                    pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                    GLOB_SC_REACQ_FLAG = 1;
                    GLOB_SC_REACQ_STAGE = 0;
                    GLOB_SC_REACQ_PRE_STEP = GLOB_SC_REACQ_CUR_STEP;
                    pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                    // Move N steps forward
                    int32_t num_steps = static_cast<int32_t>((GLOB_SC_REACQ_STAGE+1)*GLOB_SC_REACQ_STEPSIZE);
                    std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", num_steps, 100);
                    cout << result << endl;
                }
                /////////////////// END REACQUISITION /////////////////////////

            }
           
        }
   

        //TO FILE
    } else {
        Eigen::Matrix<double, 20, 3> O;
        extractToMatrix(data,O);
    }
    std::chrono::time_point<std::chrono::system_clock> end;
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - GLOB_SC_PREVIOUS;
    GLOB_SC_PREVIOUS = end;
    if (GLOB_SC_PRINT_COUNTER > 20){
        GLOB_SC_PRINT_COUNTER = 0;
        cout << "FPS: " << 1/elapsed_seconds.count() << endl;
    }
    GLOB_SC_PRINT_COUNTER++;
    return 0;
}


// FLIR Camera Server
struct SciCam: QHYCameraServer{

    SciCam() : QHYCameraServer(GroupDelayCallback){

        toml::table config = toml::parse_file(GLOB_CONFIGFILE);
        // Retrieve port and IP
        std::string CA_port = config["ScienceCamera"]["CA_port"].value_or("4100");
        std::string TS_port = config["ScienceCamera"]["TS_port"].value_or("4100");
        std::string IP = config["IP"].value_or("192.168.1.3");
        std::string TS_IP = config["ScienceCamera"]["TS_IP"].value_or("192.168.1.4");

        // Turn into a TCPString
        std::string CA_TCP = "tcp://" + IP + ":" + CA_port;
        std::string TS_TCP = "tcp://" + TS_IP + ":" + TS_port;
        
        CA_SOCKET = new commander::client::Socket(CA_TCP);
        TS_SOCKET = new commander::client::Socket(TS_TCP);

        GLOB_SC_TRACK_PERIOD = config["ScienceCamera"]["tracking_period"].value_or(100);
        GLOB_SC_SCAN_PERIOD = config["ScienceCamera"]["scanning_period"].value_or(500);


        int xref = config["ScienceCamera"]["xref"].value_or(500);
        int yref = config["ScienceCamera"]["yref"].value_or(500);

        setPixelPositions(xref,yref);
        
        GLOB_SC_WINDOW_ALPHA = config["ScienceCamera"]["window_alpha"].value_or(1.0);
        GLOB_SC_GAIN = config["ScienceCamera"]["gain"].value_or(1.0);
        int numDelays = config["ScienceCamera"]["numDelays"].value_or(6000);
        double delaySize = config["ScienceCamera"]["delaySize"].value_or(0.01);    

        GLOB_SC_V2SNR_THRESHOLD = config["ScienceCamera"]["SNRThreshold"].value_or(20.0);
        GLOB_SC_REACQ_THRESHOLD = config["ScienceCamera"]["SNRReacqThreshold"].value_or(10.0);
        GLOB_SC_REACQ_STEPSIZE = config["ScienceCamera"]["reacq_stepsize"].value_or(100);

        calcTrialDelayMat(numDelays,delaySize);

        for(int k=0;k<6;k++){
            GLOB_SC_CAL.wave_offset[k] = config["ScienceCamera"]["wave_offsets"][k].value_or(0); 
        }

        GLOB_SC_V2 = Eigen::MatrixXd::Zero(20,1);
        GLOB_SC_DELAY_AVE = Eigen::MatrixXd::Zero(numDelays,1);
        GLOB_SC_DELAY_FOREGROUND_AMP = Eigen::MatrixXd::Zero(numDelays,1);
        
        // Fringe Scanning funcs
        //GLOB_SC_SCAN_WINDOW_SIZE = config["ScienceCamera"]["FringeScanning"]["window_size"].value_or(0);
        //GLOB_SC_SCAN_SIGNAL_WIDTH = config["ScienceCamera"]["FringeScanning"]["signal_width"].value_or(0);
    }

    ~SciCam(){
        delete CA_SOCKET;
    }

    string enableDarks(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0 and GLOB_RUNNING == 0){
                ret_msg = "Changing Dark flag to: " + to_string(flag);
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_DARK_FLAG = flag;
                if (flag){
                    GLOB_DATATYPE = "DARK";
                } else {
                    GLOB_DATATYPE = "INTERFEROMETRIC";
                }
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    string enableFluxes(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0 and GLOB_RUNNING == 0){
                if (GLOB_SC_STAGE > 0){
                    string status;
                    pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                    if (flag == 0){
                        status = "off"; 
                        GLOB_DATATYPE = "INTERFEROMETRIC";
                    } else if (flag == 1){
                        status = "Dextra";
                        GLOB_DATATYPE = "DEXTRA FLUX";
                    } else if (flag == 2){
                        status = "Sinistra";
                        GLOB_DATATYPE = "SINISTRA FLUX";
                    } else {
                        status = "UNKNOWN INDEX";
                    }
                    GLOB_SC_FLUX_FLAG = flag;
                    pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                    ret_msg = "Changing Flux flag to: " + status;
                } else {
                    ret_msg = "Please save Darks first";
                }
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }
    
    string enableForeground(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0 and GLOB_RUNNING == 0){
                ret_msg = "Changing Foreground flag to: " + to_string(flag);
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_FOREGROUND_FLAG = flag;
                if (flag){
                    GLOB_DATATYPE = "FOREGROUND";
                } else {
                    GLOB_DATATYPE = "INTERFEROMETRIC";
                }
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    string calcP2VM(){
        string ret_msg;
        if (GLOB_SC_STAGE > 2){
            ret_msg = "Setting up P2VM Matrix";
            calcP2VMmain();
            if (GLOB_SC_STAGE == 3){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_STAGE = 4;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
            }
        } else if (GLOB_SC_STAGE == 0){
            ret_msg = "Please save Darks first";
        } else if (GLOB_SC_STAGE == 1){
            ret_msg = "Please save Flux Dextra first";
        } else if (GLOB_SC_STAGE == 2){
            ret_msg = "Please save Flux Sinistra first";
        }
        return ret_msg;
    }
    
    string readP2VM(){
        string ret_msg;
        readP2VMmain();
        ret_msg = "Read in P2VM Mats";
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_STAGE = 4;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        return ret_msg;
    }

    string enableFringeScan(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                if (GLOB_SC_STAGE > 4){
                    if (flag == 1){
                        ret_msg = "Enabling Fringe Scanning";
                        cout << ret_msg << endl;
                        
                        ret_msg = this->stopcam();
                        cout << ret_msg << endl;

                        // MAY NEED TO WAIT FOR HOME TO COMPLETE
                        std::string result = CA_SOCKET->send<std::string>("CA.homeSDC");

                        // Start Camera with no frames
                        pthread_mutex_lock(&GLOB_FLAG_LOCK);
                        GLOB_NUMFRAMES = 0;
                        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
                        ret_msg = this->startcam(GLOB_NUMFRAMES);
                        cout << ret_msg << endl;

                        // Start scan
                        std::string ret_msg = CA_SOCKET->send<std::string>("CA.moveSDC", -400000, GLOB_SC_SCAN_PERIOD);

                        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                        GLOB_SC_SCAN_FLAG = 1;
                        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                    } else {

                        ret_msg = "Disabling Fringe Scanning";
                        cout << ret_msg << endl;
                        ret_msg = this->stopcam();
                        cout << ret_msg << endl;

                        // Stop scan
                        std::string ret_msg = CA_SOCKET->send<std::string>("CA.moveSDC", 0, GLOB_SC_SCAN_PERIOD);
                        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                        GLOB_SC_SCAN_FLAG = 0;
                        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK); 

                    }
                    
                } else if (GLOB_SC_STAGE == 0){
                    ret_msg = "Please save Darks first";
                } else if (GLOB_SC_STAGE == 1){
                    ret_msg = "Please save Flux Dextra first";
                } else if (GLOB_SC_STAGE == 2){
                    ret_msg = "Please save Flux Sinistra first";
                } else if (GLOB_SC_STAGE == 3){
                    ret_msg = "Please make P2VM matrices first";
                } else if (GLOB_SC_STAGE == 4){
                    ret_msg = "Please save foreground first";
                }
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    string enableGDservo(int flag){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_SERVO_FLAG = flag;
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                usleep(1000);
                if (flag == 0){
                    std::string ret_msg = CA_SOCKET->send<std::string>("CA.moveSDC", 0, GLOB_SC_SCAN_PERIOD);
                }
                ret_msg = "Changing GD servo flag to: " + to_string(flag);
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    string purge(){
        string ret_msg;
        if(GLOB_CAM_STATUS == 2){
            if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
                pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
                GLOB_SC_STAGE = 0;
                
                GLOB_SC_WINDOW_INDEX = 0;
                GLOB_SC_GD = 0.0;
                GLOB_SC_V2SNR = 0.0;
                GLOB_SC_DARK_VAL = 0.0;
                GLOB_SC_TOTAL_FLUX = 0.0;

                GLOB_SC_DELAY_FOREGROUND_AMP.setZero();
                GLOB_SC_DELAY_AVE.setZero();
                GLOB_SC_V2.setZero();
                GLOB_SC_FLUX_A.setZero();
                GLOB_SC_FLUX_B.setZero();

                for (int k=0; k<20; k++){
                    GLOB_SC_P2VM_l[k].setZero();
                }
                pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
                ret_msg = "Purged global parameters";
            }else{
                ret_msg = "Camera Busy!";
            }
	    }else{
		    ret_msg = "Camera Not Connected or Currently Connecting!";
	    }
        return ret_msg;
    }

    string getV2SNRestimate(){
        double a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_V2SNR;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        string ret_msg = "V2 SNR Estimate is: " + to_string(a);
        return ret_msg;
    }

    string getGDestimate(){
        double a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_GD;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        string ret_msg = "Group Delay Estimate is: " + to_string(a);
        return ret_msg;
    }
    
    string getFlux(){
        double a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_TOTAL_FLUX;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        string ret_msg = "Total Flux is: " + to_string(a);
        return ret_msg;
    }

    bool fringeScanStatus(){
        bool a;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        a = GLOB_SC_SCAN_FLAG;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        return a;
    }

    string getV2array(){
        string ret_msg;
        json j;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        vector<double> vec (GLOB_SC_V2.data(), GLOB_SC_V2.data() + GLOB_SC_V2.size());
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        j["V2"] = vec;
        std::string s = j.dump();
        ret_msg = s;
        return ret_msg;
    }


    string getGDarray(){
        string ret_msg;
        json j;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        vector<double> vec (GLOB_SC_DELAY_AVE.data(), GLOB_SC_DELAY_AVE.data() + GLOB_SC_DELAY_AVE.size());
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        j["GroupDelay"] = vec;
        std::string s = j.dump();
        ret_msg = s;
        return ret_msg;
    }

    string setTargetandBaseline(){
        string ret_msg;
        auto coords = TS_SOCKET->send<coord>("TS.getCoordinates");
        auto target_name = TS_SOCKET->send<std::string>("TS.getTargetName");
        auto baseline = TS_SOCKET->send<double>("TS.getBaseline");
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_TARGET_NAME = target_name;
        GLOB_BASELINE = baseline;
        GLOB_RA = coords.RA;
        GLOB_DEC = coords.DEC;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        ret_msg = "Science Camera: Set Baseline to " + to_string(baseline) + " and target to " + target_name;
        return ret_msg;
    }
    
    string setGainParams(double gain, double window_alpha){
        string ret_msg;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_WINDOW_ALPHA = window_alpha;
        GLOB_SC_GAIN = gain;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        ret_msg = "Set science gain to " + to_string(gain) + " and fading memory param to " + to_string(window_alpha);
        return ret_msg;
    }
    
    string setSNRthresholds(double V2SNR_threshold, double reacq_threshold){
        string ret_msg;
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_V2SNR_THRESHOLD = V2SNR_threshold;
        GLOB_SC_REACQ_THRESHOLD = reacq_threshold;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        ret_msg = "Set V2SNR threshold to " + to_string(V2SNR_threshold) + " and reacquisition threshold to " + to_string(reacq_threshold);
        return ret_msg;
    }
    
    string setRefPixel(int xref, int yref){
        string ret_msg;
        setPixelPositions(xref,yref);
        ret_msg = this->purge();
        std::cout << ret_msg << std::endl;
        ret_msg = "Set ref pixel to " + to_string(xref) + ", " + to_string(yref) + "and purged saved arrays";
        return ret_msg;
    }
    
    string nextZaber(){
        string ret_msg;
        json j;

        // Get next FFT
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_NEXT_SCAN_FLAG = 1;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        
        // Wait until complete
        while (GLOB_SC_NEXT_SCAN_FLAG){
            usleep(100);
        }
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        for (int k=0;k<6;k++){
            vector<double> vec (GLOB_SC_SCAN_FFT_LS[k], GLOB_SC_SCAN_FFT_LS[k]+6);
            j["FFT"][k] = vec;
        }
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
        
        std::string s = j.dump();
        ret_msg = s;

        return ret_msg;
    }

    string zaberStart(){
        string ret_msg;

        // Get next FFT
        //std::string result = CA_SOCKET->send<std::string>("CA.moveSDC", -400000, 5000);
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_NEXT_SCAN_FLAG = 1;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);

        ret_msg = "Starting FFTs";

        return ret_msg;
    }

    string zaberStop(){
        string ret_msg;

        // Get next FFT
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        GLOB_SC_NEXT_SCAN_FLAG = 0;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);

        ret_msg = "Stopping FFTs";

        return ret_msg;
    }
    
    string setupZaber(){
        string ret_msg;
        int ret_val = init_fringe_scan();
        ret_msg = "Successfully inited fringe scan";
        return ret_msg;
    }

};


// Register as commander server
// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<SciCam>("SC")
        // To insterface a class method, you can use the `def` method.
        // These are part of all camera classes.
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
        // These are now specific to the science camera.
        .def("enableDarks", &SciCam::enableDarks, "Enable darks")
        .def("enableFluxes", &SciCam::enableFluxes, "Enable fluxes")
        .def("enableForeground", &SciCam::enableForeground, "Enable foreground")
        .def("enableGDservo", &SciCam::enableGDservo, "Start/Stop GD servo")
        .def("purge", &SciCam::purge, "Purge saved P2VM related arrays")
        .def("calcP2VM", &SciCam::calcP2VM, "Calculate P2VM matrices")
        .def("readP2VM", &SciCam::readP2VM, "Read P2VM matrices")
        .def("enableFringeScan", &SciCam::enableFringeScan, "Enable fringe scanning")
        .def("fringeScanStatus", &SciCam::fringeScanStatus, "Are we still fringe scanning?")
        .def("getGDarray", &SciCam::getGDarray, "Get current group delay envelope")
        .def("getGDestimate", &SciCam::getGDestimate, "Get current group delay estimate")
        .def("getFlux", &SciCam::getFlux, "Get current total flux")
        .def("getV2array", &SciCam::getV2array, "Get V2 array per pixel")
        .def("getV2SNRestimate", &SciCam::getV2SNRestimate, "Get V2 SNR estimate")
        .def("setTargetandBaseline", &SciCam::setTargetandBaseline, "Set target and baseline info for FITS")
        .def("setGains", &SciCam::setGainParams, "Set gain and fading memory parameters")
        .def("setSNRs", &SciCam::setSNRthresholds, "Set SNR thresholds")
        .def("setRefPix", &SciCam::setRefPixel, "Set reference pixel position (purges saved arrays)")
        // The Zaber stage is only used for lab alignment.
        .def("zaberFunc", &SciCam::nextZaber, "Request and get new SNR estimate from fringe scanning")
        .def("setupZaber", &SciCam::setupZaber, "Request and get new SNR estimate from fringe scanning")
        .def("zaberStart", &SciCam::zaberStart, "Request and get new SNR estimate from fringe scanning")
        .def("zaberStop", &SciCam::zaberStop, "Request and get new SNR estimate from fringe scanning");

}
