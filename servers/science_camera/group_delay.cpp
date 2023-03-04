// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>
#include <iostream>
#include "setup.hpp"

using Cd = std::complex<double>;

const double PI = 3.14159265;
const Cd If(0.,1.);

Eigen::MatrixXcd GLOB_SC_DELAYMAT;


int calcTrialDelayMat(int numDelays, double delaySize, double* wavelengths){

    Eigen::MatrixXcd phasors = Eigen::MatrixXcd::Zero(numDelays,20);

    double edge = static_cast<double>(numDelays)*delaySize*0.5;

    Eigen::ArrayXf delays = Eigen::ArrayXf::LinSpaced(numDelays, -edge, edge);

    std::cout << d<< std::endl;

    for(int k=0;k<numDelays;k++){
        for(int l=0;l<10;l++){
            Cd num = 2*PI*If*delays(k)/wavelengths[l];
            phasors(k,l) = num;
            phasors(k,l+10) = num;
        }
    }

    GLOB_SC_DELAYMAT = phasors.array().exp();

    std::cout << d<< std::endl;

    return 0;

}


int calcGroupDelay() {
    // An example of Einstein summation

    Eigen::Matrix<T, 20, 3> O;
     for(int k=0;k<20;k++){
        O.row(k) << 1.*(k+1),2.*(1+k),3.*(1+2*k);
    }   

    //O.transposeInPlace();

    Eigen::Matrix<T, 20, 3> R;
    for(int k=0;k<20;k++){
        Eigen::Matrix<T, 3, 1> O_i = O.row(k);
        R.row(k) = *GLOB_SC_P2VM_l[k]*O_i;
    }

    g = (R.col(0) + If*R.col(1));
    g = g.array()*((R.col(2)).array().inverse());

    Eigen::Matrix<T,1000,1> d = (GLOB_SC_DELAYMAT*g).cwiseAbs2();

    std::cout << d<< std::endl;
    // Output the results


    return 0;
}