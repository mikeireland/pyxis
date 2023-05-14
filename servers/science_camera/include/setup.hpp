// Function to calculate pixel positions

// Calculate P2VM matrix

// Calibrate wavelength scale

#include <Eigen/Dense>

extern Eigen::Matrix<std::complex<double>,3,3> *GLOB_SC_P2VM_l[20];
extern Eigen::Matrix<std::complex<double>,20,3> GLOB_SC_FLUX_A;
extern Eigen::Matrix<std::complex<double>,20,3> GLOB_SC_FLUX_B;
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

int calcP2VMMat();
void extractToMatrix(unsigned short* data, Eigen::Matrix* O);
int findPixelPositions(int xref, int yref);