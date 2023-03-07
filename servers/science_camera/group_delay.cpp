// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>
#include <iostream>
#include "setup.hpp"

using Cd = std::complex<double>;

const double PI = 3.14159265;
const Cd If(0.,1.);

Eigen::MatrixXcd GLOB_SC_DELAYMAT;
Eigen::MatrixXd GLOB_SC_DELAY_AMP;
double GLOB_SC_GD;

Eigen::Matrix<Cd, 20, 3> O;
Eigen::Matrix<Cd, 20, 3> R;
Eigen::Matrix<Cd, 20, 1> g;

Eigen::ArrayXcd delays;


int calcTrialDelayMat(int numDelays, double delaySize){

    Eigen::MatrixXcd phasors = Eigen::MatrixXcd::Zero(numDelays,20);

    double edge = static_cast<double>(numDelays)*delaySize*0.5;

    delays = Eigen::ArrayXcd::LinSpaced(numDelays, -edge, edge);

    for(int k=0;k<numDelays;k++){
        for(int l=0;l<10;l++){
            Cd num = 2*PI*If*delays(k)/GLOB_SC_WAVELENGTHS[l];
            phasors(k,l) = num;
            phasors(k,l+10) = num;
        }
    }

    GLOB_SC_DELAYMAT = phasors.array().exp();

    return 0;

}


int calcGroupDelay(unsigned short* data) {


     for(int k=0;k<10;k++){
        O.row(k) << data[GLOB_SC_CAL.pos_p1_A,GLOB_SC_CAL.pos_wave+k],
                    data[GLOB_SC_CAL.pos_p1_B,GLOB_SC_CAL.pos_wave+k],
                    data[GLOB_SC_CAL.pos_p1_C,GLOB_SC_CAL.pos_wave+k];
        O.row(k+10) << data[GLOB_SC_CAL.pos_p2_A,GLOB_SC_CAL.pos_wave+k],
                    data[GLOB_SC_CAL.pos_p2_B,GLOB_SC_CAL.pos_wave+k],
                    data[GLOB_SC_CAL.pos_p2_C,GLOB_SC_CAL.pos_wave+k];
    }   

    for(int k=0;k<20;k++){
        Eigen::Matrix<Cd, 3, 1> O_i = O.row(k);
        R.row(k) = *GLOB_SC_P2VM_l[k]*O_i;
    }

    g = (R.col(0) + If*R.col(1));
    g = g.array()*((R.col(2)).array().inverse());

    GLOB_SC_DELAY_AMP = (GLOB_SC_DELAYMAT*g).cwiseAbs2().real();

    Eigen::Index maxRow, maxCol;
    double maxAmp = GLOB_SC_DELAY_AMP.maxCoeff(&maxRow,&maxCol);
    GLOB_SC_GD = delays(maxRow,maxCol).real();

    return 0;
}