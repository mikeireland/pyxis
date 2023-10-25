#ifndef _SC_GROUPDELAY_
#define _SC_GROUPDELAY_

/* Functions dealing with calculating the group delay */

#include <Eigen/Dense>

// Matrix of trial delays for each polarisation and wavelength (10 channels)
extern Eigen::MatrixXcd GLOB_SC_DELAYMAT;

// Foreground amplitude to remove from the delays
extern Eigen::MatrixXd GLOB_SC_DELAY_FOREGROUND_AMP;

// Average of the delays
extern Eigen::MatrixXd GLOB_SC_DELAY_AVE;

// V2 array
extern Eigen::MatrixXd GLOB_SC_V2;

extern int GLOB_SC_WINDOW_INDEX; // Is this the first set of data (for GD averaging)
extern double GLOB_SC_WINDOW_ALPHA; // Alpha parameter for fading memory
extern double GLOB_SC_GD; // Group delay estimate
extern double GLOB_SC_V2SNR; // V2 SNR estimate

/* 
Calculates the matrix of all trial delays vs wavelengths (10) and polarisations (2)
Inputs:
    numDelays - How many trial delays to use?
    delaySize - What is the spacing between delays (in um)?
Output:
    Saves the trial delays in GLOB_SC_DELAYMAT, with the first 10 rows being pol 1, 
    and the second 10 rows being pol 2
*/
int calcTrialDelayMat(int numDelays, double delaySize);

/*
Function to estimate the foreground fringe envelope of the science camera data (i.e 
taking frames where there is injection, but no fringes)
Inputs:
    data - raw frame
Output:
    Saves calculated foreground amplitude in GLOB_SC_DELAY_FOREGROUND_AMP
*/
int calcForeground(unsigned short* data);

/*
Main function to estimate the group delay from a frame of the science camera
Inputs:
    data - raw science camera frame
Outputs
    Saves relevant fringe envelope averages in GLOB_SC_DELAY_AVE, 
    estimated V2 SNR in GLOB_SC_V2SNR,
    and estimated group delay in GLOB_SC_GD
*/
int calcGroupDelay(unsigned short* data);

#endif // _SC_GROUPDELAY_
