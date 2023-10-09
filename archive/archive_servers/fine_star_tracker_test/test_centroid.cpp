
#include <fmt/core.h>
#include <iostream>
#include "image.hpp"
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib>

using namespace std;

struct centroid {
    double x;
    double y;
};

centroid CalcStarPosition(cv::Mat img, int height, int width){

    // Function to take image array and find the star position
    static image::ImageProcessSubMatInterpSingle ipb(height,width);

    auto p = ipb(img);

    centroid result;
    
    result.x = p.x;
    result.y = p.y;
    //result.x = rand();
    //result.y = rand();

    return result;

}


// Return 1 if error!
int FST_Callback (unsigned short* data){

    
    int height = 1080;
    int width = 1440;
    
    cv::Mat img (height,width,CV_16U,data);
    centroid position = CalcStarPosition(img,height,width);

    //cout << "Sending position" << endl;
    
    cout << position.x << ", " << position.y << endl;

    return 0;
}


int main(int argc, char* argv[]) {

    cv::Mat img;
    img = cv::imread("mystery2_2.png",cv::IMREAD_UNCHANGED);
    
    std::vector<uint16_t>array;
    array.assign((uint16_t*)img.datastart, (uint16_t*)img.dataend);


    uint16_t* img2 = &array[0];
    
    FST_Callback(img2);
    
}
