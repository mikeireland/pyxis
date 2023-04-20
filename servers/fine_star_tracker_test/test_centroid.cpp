
#include <fmt/core.h>
#include <iostream>
#include "FLIRcamServerFuncs.h"
#include <pthread.h>
#include "globals.h"
#include "image.hpp"
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdlib>
#include "centroiders.hpp"

struct centroid {
    double x;
    double y;
};

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

    int height = ;
    int width = ;

    cv::Mat img (height,width,CV_16U,data);
    img.convertTo(img, CV_8U, 0.00390625);
    uchar * img2 = img.isContinuous()? img.data: img.clone().data;

    lost::CenterOfGravityAlgorithm CA;

    lost::Stars s = CA.Go(img2,3072, 2048);

    centroid position;
    position.x = s[0].position.x;
    position.y = s[0].position.y;

    //centroid position = CalcStarPosition(img);

    cout << "Sending position" << endl;
    
    cout << position.x << ", " << position.y << endl;

    return 0;
}


int main(int argc, char* argv[]) {


    img = cv::imread("test.png",cv::IMREAD_GRAYSCALE )
    uint_16 * img2 = img.isContinuous()? img.data: img.clone().data;
    cout << img2 << endl;

}
