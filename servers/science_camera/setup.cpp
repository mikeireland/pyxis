// Function to calculate pixel positions
// Calculate P2VM matrix
// Calibrate wavelength scale

#include <Eigen/Dense>
#include "setup.hpp"
using Cd = std::complex<double>;

Eigen::Matrix<Cd,3,3> *GLOB_SC_P2VM_l[20];
Eigen::Array<Cd,20,3> GLOB_SC_FLUX_A = Eigen::Array<Cd,20,3>::Zero();
Eigen::Array<Cd,20,3> GLOB_SC_FLUX_B = Eigen::Array<Cd,20,3>::Zero();
double GLOB_SC_DARK_VAL = 0;

SC_calibration GLOB_SC_CAL;

int addToFlux(unsigned short* data, int flux_flag){

    Eigen::Matrix<Cd, 20, 3> O;
    
    extractToMatrix(&data,&O);

    if (flux_flag == 1){
        GLOB_SC_FLUX_A += O.array();
    } else if (flux_flag == 2){
        GLOB_SC_FLUX_B += O.array();
    } else {
        return 1;
    }
    return 0;
}

int measureDark(unsigned short* data){
    int size = GLOB_IMSIZE;
    float total;
    for(int i=0;i<size; i++){
        total += data[i];
    }
    GLOB_SC_DARK_VAL = total/size;

    return 0;
}


// Temp function to create P2VM matrix. In reality, need a proper calibration function to do this.
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


void extractToMatrix(unsigned short* data, Eigen::Matrix* O) {
    
     // From the frame, extract pixel positions into O(utput) matrix
     for(int k=0;k<10;k++){
        O.row(k) << data[GLOB_SC_CAL.pos_p1_A,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL,
                    data[GLOB_SC_CAL.pos_p1_B,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL,
                    data[GLOB_SC_CAL.pos_p1_C,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL;
        O.row(k+10) << data[GLOB_SC_CAL.pos_p2_A,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL,
                    data[GLOB_SC_CAL.pos_p2_B,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL,
                    data[GLOB_SC_CAL.pos_p2_C,GLOB_SC_CAL.pos_wave+k]-GLOB_SC_DARK_VAL;
    }   
}

// WAVELENGTHS: 0.6063 0.6186 0.6316 0.6454 0.66 0.6755 0.6918 0.7092 0.7277 0.7473

// Function to get the reference pixel positions.
// xref is location of 660nm (offset of -4 if red end on the right, or +5 if on the left)
// for 630nm, offset of -2 for red end on the right, or +7 if on the left
// yref is location of first row

int setPixelPositions(int xref, int yref) {

    GLOB_SC_CAL.pos_wave = xref - 4;

    GLOB_SC_CAL.pos_p1_A = yref;
    GLOB_SC_CAL.pos_p2_A = yref + 3;
    GLOB_SC_CAL.pos_p1_B = yref + 7;
    GLOB_SC_CAL.pos_p2_B = yref + 12;
    GLOB_SC_CAL.pos_p1_C = yref + 15;
    GLOB_SC_CAL.pos_p2_C = yref + 18;

    double temp_waves [] = {0.6063, 0.6186, 0.6316, 0.6454, 0.66, 0.6755, 0.6918, 0.7092, 0.7277, 0.7473};

    for(int k=0;k<10;k++){
        GLOB_SC_CAL.wavelengths[k] = temp_waves[k];
    }

    return 0;
}