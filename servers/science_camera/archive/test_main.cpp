#include <fstream>
#include <fmt/core.h>
#include <iostream>
#include "toml.hpp"
#include "globals.h"
#include "setup.hpp"
#include "group_delay.hpp"
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib>

#include <chrono>
#include <ctime>

using namespace std;


int GLOB_SC_DARK_FLAG;
int GLOB_SC_FLUX_FLAG;
int GLOB_SC_SCAN_FLAG;


// Return 1 if error!
int FST_Callback (unsigned short* data){
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

    string filename = "config/defaultLocalConfig.toml";
    toml::table config = toml::parse_file(filename);

    int xref = config["ScienceCamera"]["xref"].value_or(500);
    int yref = config["ScienceCamera"]["yref"].value_or(500);
    
    setPixelPositions(xref,yref);

    int numDelays = config["ScienceCamera"]["numDelays"].value_or(1000);
    double delaySize = config["ScienceCamera"]["delaySize"].value_or(0.02);    

    calcTrialDelayMat(numDelays,delaySize);
    
    GLOB_IMSIZE = 10240;
    GLOB_WIDTH = 80;

    for(int k=0;k<20;k++){
        GLOB_SC_P2VM_SIGNS[k] = config["ScienceCamera"]["signs"][k].value_or(1); 
    }

    for(int k=0;k<6;k++){
        GLOB_SC_CAL.wave_offset[k] = config["ScienceCamera"]["wave_offsets"][k].value_or(0); 
    }

    GLOB_SC_V2 = Eigen::MatrixXd::Zero(20,1);
    GLOB_SC_DELAY_AVE = Eigen::MatrixXd::Zero(numDelays,1);

    cv::Mat img;
    std::vector<uint16_t>array;
    uint16_t* img2;
    /*
    img = cv::imread("dark.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    GLOB_SC_DARK_FLAG = 1;
    FST_Callback(img2);
    
    img = cv::imread("f11.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    GLOB_SC_DARK_FLAG = 0;
    GLOB_SC_FLUX_FLAG=1;
    FST_Callback(img2);
    
    img = cv::imread("f12.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    img = cv::imread("f13.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    img = cv::imread("f14.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    img = cv::imread("f15.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    //cout << GLOB_SC_FLUX_A << endl;
    
    img = cv::imread("f21.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    GLOB_SC_FLUX_FLAG=2;
    FST_Callback(img2);
    
    img = cv::imread("f22.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    img = cv::imread("f23.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    img = cv::imread("f24.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    FST_Callback(img2);
    
    //img = cv::imread("f25.png",cv::IMREAD_UNCHANGED);
    //array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    //img2 = &array[0];
    //FST_Callback(img2);
    
    //cout << GLOB_SC_FLUX_B << endl;
    */
    //calcP2VMmain();
    readP2VMmain();
    
    //cout << GLOB_SC_P2VM_l[0] << endl;
    //cout << GLOB_SC_P2VM_l[6] << endl;
    cout << GLOB_SC_FLUX_A << endl;
    cout << GLOB_SC_FLUX_B << endl;
    
    img = cv::imread("fringe1.png",cv::IMREAD_UNCHANGED);
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);
    img2 = &array[0];
    GLOB_SC_FLUX_FLAG=0;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    FST_Callback(img2);
    
    //cout << GLOB_SC_V2 << endl;
    cout << GLOB_SC_V2SNR << endl;
    cout << GLOB_SC_GD << endl;
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << elapsed_seconds.count()*1000 << endl;
    
    
    
    
}
