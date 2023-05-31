#ifndef _SC_SETUP_
#define _SC_SETUP_

// Function to calculate pixel positions

// Calculate P2VM matrix

// Calibrate wavelength scale

#include <Eigen/Dense>

using Cd = std::complex<double>;

extern const Cd I;

extern Eigen::Matrix<Cd,3,3> GLOB_SC_P2VM_l[20];
extern int GLOB_SC_P2VM_SIGNS[20];
extern Eigen::Array<double,20,3> GLOB_SC_FLUX_A;
extern Eigen::Array<double,20,3> GLOB_SC_FLUX_B;
extern double GLOB_SC_DARK_VAL;

struct SC_calibration {

    double wavelengths[10];

    int pos_wave;

    int pos_p1_A;
    int pos_p2_A;
    int pos_p1_B;
    int pos_p2_B;
    int pos_p1_C;
    int pos_p2_C;

};

extern SC_calibration GLOB_SC_CAL;

int calcP2VMmain();
int measureDark(unsigned short* data);
int addToFlux(unsigned short* data, int flux_flag);
void extractToMatrix(unsigned short* data, Eigen::Matrix<double, 20, 3> & O);
int setPixelPositions(int xref, int yref);

#endif // _SC_SETUP_
