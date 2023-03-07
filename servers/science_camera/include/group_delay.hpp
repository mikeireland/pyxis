// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>

extern Eigen::MatrixXcd GLOB_SC_DELAYMAT;
extern Eigen::MatrixXd GLOB_SC_DELAY_AMP;
extern double GLOB_SC_GD;

int calcTrialDelayMat(int numDelays, double delaySize);


int calcGroupDelay();
