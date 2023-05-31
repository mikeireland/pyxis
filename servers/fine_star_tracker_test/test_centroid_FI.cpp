
#include <fmt/core.h>
#include <iostream>
#include "centroid.hpp"
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib>

using namespace std;

centroid GLOB_FI_DEXTRA_TARGET_CENTROID;
centroid GLOB_FI_DEXTRA_CURRENT_CENTROID;
centroid GLOB_FI_DEXTRA_DIFF_POSITION;

int GLOB_FI_TIPTILTSERVO_FLAG;
int GLOB_FI_SET_TARGET_FLAG;

int GLOB_FI_INTERP_SIZE;
int GLOB_FI_GAUSS_RAD;
int GLOB_FI_WINDOW_SIZE;
double GLOB_FI_CENTROID_GAIN;

cv::Mat GLOB_FI_CENTROID_WEIGHTS;


// Return 1 if error!
int FibreInjectionCallback (unsigned short* data){
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    int height = 1080;
    int width = 1440;

    cv::Mat img (height,width,CV_16U,data);

    if (GLOB_FI_TIPTILTSERVO_FLAG){

        cv::Point2i DextraCentre(GLOB_FI_DEXTRA_CURRENT_CENTROID.x, GLOB_FI_DEXTRA_CURRENT_CENTROID.y);

        auto DextraP = centroid_funcs::getCentroidWCOG(img, DextraCentre, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_INTERP_SIZE, GLOB_FI_CENTROID_GAIN);
        //auto DextraP = centroid_funcs::getCentroidCOG(img, DextraCentre, GLOB_FI_INTERP_SIZE);
        
        GLOB_FI_DEXTRA_CURRENT_CENTROID.x = DextraP.x;
        GLOB_FI_DEXTRA_CURRENT_CENTROID.y = DextraP.y;

        GLOB_FI_DEXTRA_DIFF_POSITION.x = GLOB_FI_DEXTRA_TARGET_CENTROID.x - DextraP.x;
        GLOB_FI_DEXTRA_DIFF_POSITION.y = GLOB_FI_DEXTRA_TARGET_CENTROID.y - DextraP.y;
        

    } else {

        cv::Point2i DextraCentre(GLOB_FI_DEXTRA_TARGET_CENTROID.x, GLOB_FI_DEXTRA_TARGET_CENTROID.y);

        auto DextraP = centroid_funcs::windowCentroidWCOG(img, GLOB_FI_INTERP_SIZE, GLOB_FI_GAUSS_RAD, DextraCentre, 
                                        GLOB_FI_WINDOW_SIZE, GLOB_FI_CENTROID_WEIGHTS, GLOB_FI_CENTROID_GAIN);
        //auto DextraP = centroid_funcs::windowCentroidCOG(img, GLOB_FI_INTERP_SIZE, GLOB_FI_GAUSS_RAD, DextraCentre, 
        //                        GLOB_FI_WINDOW_SIZE);

        GLOB_FI_DEXTRA_CURRENT_CENTROID.x = DextraP.x;
        GLOB_FI_DEXTRA_CURRENT_CENTROID.y = DextraP.y;

        GLOB_FI_DEXTRA_DIFF_POSITION.x = GLOB_FI_DEXTRA_TARGET_CENTROID.x - DextraP.x;
        GLOB_FI_DEXTRA_DIFF_POSITION.y = GLOB_FI_DEXTRA_TARGET_CENTROID.y - DextraP.y;

        if (GLOB_FI_SET_TARGET_FLAG){
            GLOB_FI_DEXTRA_TARGET_CENTROID.x = GLOB_FI_DEXTRA_CURRENT_CENTROID.x;
            GLOB_FI_DEXTRA_TARGET_CENTROID.y = GLOB_FI_DEXTRA_CURRENT_CENTROID.y;
            GLOB_FI_SET_TARGET_FLAG = 0;
        }
    }
    
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    cout << "FPS: " << 1/elapsed_seconds.count() << endl;
    
    cout << "Current Position: " << GLOB_FI_DEXTRA_CURRENT_CENTROID.x << ", " << GLOB_FI_DEXTRA_CURRENT_CENTROID.y << endl;
        
    cout << "Target Position: " << GLOB_FI_DEXTRA_TARGET_CENTROID.x << ", " << GLOB_FI_DEXTRA_TARGET_CENTROID.y << endl;
        
    cout << "Differential Position: " << GLOB_FI_DEXTRA_DIFF_POSITION.x << ", " << GLOB_FI_DEXTRA_DIFF_POSITION.y << endl;
    return 0;
}

int main(int argc, char* argv[]) {

    GLOB_FI_DEXTRA_CURRENT_CENTROID.x = 550;
    GLOB_FI_DEXTRA_CURRENT_CENTROID.y = 483;

    GLOB_FI_DEXTRA_TARGET_CENTROID.x = 550;
    GLOB_FI_DEXTRA_TARGET_CENTROID.y = 483;
    GLOB_FI_DEXTRA_DIFF_POSITION.x = 0.0;
    GLOB_FI_DEXTRA_DIFF_POSITION.y = 0.0;
    
    GLOB_FI_INTERP_SIZE = 11;
    GLOB_FI_GAUSS_RAD = 9;
    GLOB_FI_WINDOW_SIZE = 20;
    GLOB_FI_CENTROID_GAIN = 1.0;
    double sigma = 4;
    
    GLOB_FI_TIPTILTSERVO_FLAG = 0;
    GLOB_FI_SET_TARGET_FLAG = 0;
    
    GLOB_FI_CENTROID_WEIGHTS = centroid_funcs::weightFunction(GLOB_FI_INTERP_SIZE, sigma);

    cv::Mat img;
    img = cv::imread("/home/pyxisuser/GitRepos/pyxis/servers/fine_star_tracker_test/tiptilt_300523/random_spots-05302023163315-8.pgm",cv::IMREAD_UNCHANGED);
    img.convertTo(img,CV_16U);
    
    std::vector<uint16_t>array;
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);


    uint16_t* img2 = &array[0];
    
    FibreInjectionCallback(img2);
    
}
