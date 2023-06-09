#include <fstream>
#include <fmt/core.h>
#include <iostream>
#include "toml.hpp"
#include "setup.hpp"
#include "group_delay.hpp"
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib>

using namespace std;


int GLOB_SC_DARK_FLAG;
int GLOB_SC_FLUX_FLAG;
int GLOB_SC_SCAN_FLAG;

pthread_mutex_t GLOB_SC_FLAG_LOCK;

// Return 1 if error!
int GroupDelayCallback (unsigned short* data){
    int ret_val;
    if (GLOB_SC_DARK_FLAG){
        ret_val = measureDark(data);
        
    } else if (GLOB_SC_FLUX_FLAG == 1){
        ret_val = addToFlux(data,1);
    } else if (GLOB_SC_FLUX_FLAG == 2){
        ret_val = addToFlux(data,2);
    } else {
        ret_val = calcGroupDelay(data);
    }
    return 0;
}


int main(int argc, char* argv[]) {

    string filename = "filename here"
    toml::table config = toml::parse_file(filename);

    int xref = config["ScienceCamera"]["xref"].value_or(500);
    int yref = config["ScienceCamera"]["yref"].value_or(500);

    setPixelPositions(xref,yref);

    int numDelays = config["ScienceCamera"]["numDelays"].value_or(1000);
    double delaySize = config["ScienceCamera"]["delaySize"].value_or(1);    

    calcTrialDelayMat(numDelays,delaySize);

    for(int k=0;k<20;k++){
        GLOB_SC_P2VM_SIGNS[k] = config["ScienceCamera"]["signs"][k].value_or(1); 
    }

    for(int k=0;k<6;k++){
        GLOB_SC_CAL.wave_offset[k] = config["ScienceCamera"]["wave_offsets"][k].value_or(0); 
    }

    GLOB_SC_V2 = Eigen::MatrixXd::Zero(20,1);
    GLOB_SC_DELAY_AVE = Eigen::MatrixXd::Zero(numDelays,1);


    cv::Mat img_dark, img_flat1, img_flat2, img_data;
    
    img = cv::imread("mystery2_2.png",cv::IMREAD_UNCHANGED);
    std::vector<uint16_t>array;
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    uint16_t* img2 = &array[0];
    
    FST_Callback(img2);
    
}
