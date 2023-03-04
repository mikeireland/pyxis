#include <Eigen/Dense>
#include <iostream>
#include "group_delay.hpp"
#include "setup.hpp"



int main() {
    // An example of Einstein summation
    int numDelays = 1000;
    double delaySize = 20; //in nm
    
    double* wavelengths[10] = {610.,625.,640.,655.,670.,685.,700.,715.,730.,745.};
    
    int retVal;

    retVal = calcTrialDelayMat(numDelays, delaySize, wavelengths);

    retVal = calcP2VMMat();

    retVal = calcGroupDelay();

    return retVal;

}