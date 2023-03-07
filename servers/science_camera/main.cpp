#include <Eigen/Dense>
#include <iostream>
#include "group_delay.hpp"
#include "setup.hpp"
#include <chrono>

using namespace std::chrono;


int main() {
    // An example of Einstein summation
    int numDelays = 1000;
    double delaySize = 5; //in nm
    
    int retVal;

    retVal = wavelengthCal();

    retVal = calcTrialDelayMat(numDelays, delaySize);

    retVal = calcP2VMMat();

    auto start = high_resolution_clock::now();
    for (int k=0;k<1000;k++){
        retVal = calcGroupDelay();
        std::cout << GLOB_SC_GD << std::endl;
    }
    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop-start);

    std::cout << duration.count()/1000 <<std::endl;

    return retVal;

}