// Function to calculate pixel positions

// Calculate P2VM matrix

// Calibrate wavelength scale

#include <Eigen/Dense>
using Cd = std::complex<double>;

Eigen::MatrixXcd *GLOB_SC_P2VM_l;


int calcP2VMMat() {

    for(int k=0;k<20;k++){
        Eigen::Matrix<Cd,3,3> M;
        k+=1;
        M << 1.*k,2.*k,3.*k,4.*k,5.*k,6.*k,7.*k,8.*k,9.*k;
        k-=1;
        GLOB_SC_P2VM_l[k] = &M;
    }

}