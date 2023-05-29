// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>
#include <iostream>
#include "setup.hpp"

using Cd = std::complex<double>;

const double PI = 3.14159265;
const Cd If(0.,1.);

Eigen::MatrixXcd GLOB_SC_DELAYMAT;
Eigen::MatrixXd GLOB_SC_DELAY_CURRENT_AMP;
Eigen::MatrixXd GLOB_SC_DELAY_AVE;

int GLOB_SC_WINDOW_INDEX = 0;
double GLOB_SC_WINDOW_ALPHA;

double GLOB_SC_GD = 0.0;
double GLOB_SC_V2SNR = 0.0;

Eigen::ArrayXcd delays;


// Calculates the matrix of all trial delays vs wavelengths
int calcTrialDelayMat(int numDelays, double delaySize){

    Eigen::MatrixXcd phasors = Eigen::MatrixXcd::Zero(numDelays,20);

    double edge = static_cast<double>(numDelays)*delaySize*0.5;

    delays = Eigen::ArrayXcd::LinSpaced(numDelays, -edge, edge);

    for(int k=0;k<numDelays;k++){
        for(int l=0;l<10;l++){
            Cd num = 2*PI*If*delays(k)/GLOB_SC_CAL.wavelengths[l];
            phasors(k,l) = num;
            phasors(k,l+10s) = num;
        }
    }

    GLOB_SC_DELAYMAT = phasors.array().exp();

    return 0;

}


// Main function to take in a frame and calculate the group delay
int calcGroupDelay(unsigned short* data) {
    
    Eigen::Matrix<Cd, 20, 3> O;
    Eigen::Matrix<Cd, 20, 3> V;
    Eigen::Matrix<Cd, 20, 1> g;
    Eigen::Matrix<Cd, 20, 1> V2;
    Eigen::Matrix<Cd, 20, 1> SNR;
    
    extractToMatrix(&data,&O);

    // Convert to V(isibilities) via P2VM
    for(int k=0;k<10;k++){
        Eigen::Matrix<Cd, 3, 1> O_i = O.row(k);
        V.row(k) = *GLOB_SC_P2VM_l[k]*O_i;
    }

    // Get complex coherence vector (g) as a function of wavelength
    g = (V.col(0) + If*V.col(1));
    g = g.array()*((V.col(2)).array().inverse());

    V2 = g.array().abs2();
    SNR = V2*(V.col(2)).array();

    GLOB_SC_V2SNR = SNR.norm();

    // Fourier transform sampling by multiplying by trial delay matrix
    GLOB_SC_DELAY_AMP = (GLOB_SC_DELAYMAT*g).cwiseAbs2().real();
    
    //Moving average/fading memory
    if not (GLOB_SC_WINDOW_INDEX){
        GLOB_SC_DELAY_AVE = GLOB_SC_DELAY_AMP;
        GLOB_SC_WINDOW_INDEX = 1;
    } else{
        GLOB_SC_DELAY_AVE = GLOB_SC_WINDOW_ALPHA*GLOB_SC_DELAY_AMP + (1.0-GLOB_SC_WINDOW_ALPHA)*GLOB_SC_DELAY_AVE;
    }

    // Extract group delay from maximum of the fourier transform
    Eigen::Index maxRow, maxCol;
    double maxAmp = GLOB_SC_DELAY_AVE.maxCoeff(&maxRow,&maxCol);
    GLOB_SC_GD = delays(maxRow,maxCol).real();

    return 0;
}
