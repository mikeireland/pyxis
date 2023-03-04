// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>

extern Eigen::MatrixXcd GLOB_SC_DELAYMAT;


int calcTrialDelayMat(int numDelays, double delaySize, double* wavelengths);


int calcGroupDelay();