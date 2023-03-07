// Function to calculate pixel positions
// Calculate P2VM matrix
// Calibrate wavelength scale

#include <Eigen/Dense>
#include "setup.hpp"
using Cd = std::complex<double>;

Eigen::Matrix<Cd,3,3> *GLOB_SC_P2VM_l[20];

SC_calibration GLOB_SC_CAL;

int calcP2VMMat() {

    for(int k=0;k<20;k++){
        Eigen::Matrix<Cd,3,3> *M = new Eigen::Matrix<Cd,3,3>;
        k+=1;
        *M << 1.*k,2.*k,3.*k,4.*k,5.*k,6.*k,7.*k,8.*k,9.*k;
        k-=1;
        GLOB_SC_P2VM_l[k] = M;
    }

    return 0;

}

int wavelengthCal() {

    for(int k=0;k<10;k++){
        GLOB_SC_CAL.wavelengths[k] = 15.*k+600.;
    }

    return 0;
}

int findPixelPositions() {

    GLOB_SC_CAL.pos_wave = 1.;

    GLOB_SC_CAL.pos_p1_A = 1.;
    GLOB_SC_CAL.pos_p2_A = 1.;
    GLOB_SC_CAL.pos_p1_B = 1.;
    GLOB_SC_CAL.pos_p2_B = 1.;
    GLOB_SC_CAL.pos_p1_C = 1.;
    GLOB_SC_CAL.pos_p2_C = 1.;

    return 0;
}

int saveCalData() {

    return 0;
}