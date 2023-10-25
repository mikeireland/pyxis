#ifndef _SC_SETUP_
#define _SC_SETUP_

#include <Eigen/Dense>
#include <string>

using Cd = std::complex<double>;

/* Functions to help setup the use of the science camera processing */

extern pthread_mutex_t GLOB_SC_FLAG_LOCK;

extern const Cd I;

// list of all P2VM Matrices (for each polarisation (2) and wavelength channel (10))
extern Eigen::Matrix<Cd,3,3> GLOB_SC_P2VM_l[20];

// Array of fluxes of only Dextra input
extern Eigen::Array<double,20,3> GLOB_SC_FLUX_A;

// Array of fluxes of only Sinistra input
extern Eigen::Array<double,20,3> GLOB_SC_FLUX_B;

// Dark value
extern double GLOB_SC_DARK_VAL;

// Total flux value
extern double GLOB_SC_TOTAL_FLUX;

// Struct to hold the reference pixel positions and wavelengths
struct SC_calibration {

    double wavelengths[10]; // Array of wavelengths
    int wave_offset[6]; // Relative offset of the reference wavelength pixel for each of the other outputs (i.e element 0 should be 0)

    int pos_wave; // Reference pixel position of the FIRST of the wavelengths, left to right (i.e it will extract 10 pixels from this location)

    // Row positions of each of the outputs (using the TOP of the two rows that will be summed together)
    int pos_p1_A;
    int pos_p2_A;
    int pos_p1_B;
    int pos_p2_B;
    int pos_p1_C;
    int pos_p2_C;

};

// Master calibration struct
extern SC_calibration GLOB_SC_CAL;

/*
Function to calculate the P2VM matrix for each polarisation (2) and wavlength channel (10). Uses the relevant flux ratios
Also saves the calibration fluxes and dark used to calculate the matrices to a file
Saves each one as an element in the GLOB_SC_P2VM_l list
Inputs:
    P2VM_file - filename of the saved calibration data
*/
int calcP2VMmain(std::string P2VM_file);

/*
Function to read in the calibration arrays and calculate the P2VMs from a file
Inputs:
    P2VM_file - filename of the saved calibration data
*/
int readP2VMmain(std::string P2VM_file);

/*
Function to measure and save a science camera dark frame. Simply takes the mean of the entire frame
Inputs:
    data - raw science camera data
Outputs:
    Saves the data in GLOB_SC_DARK_VAL
*/
int measureDark(unsigned short* data);

/*
Function to add the tricoupler flux to a vector when only one input is injected
Inputs
    data - raw camera frame
    flux_flag - which input is injected? (1 for Dextra, 2 for Sinistra)
Outputs:
    Saves added flux values to GLOB_SC_FLUX_Y (where Y is either A or B depending on the deputy)
    Returns 1 on error (for bad flux_flag value)
*/
int addToFlux(unsigned short* data, int flux_flag);

/*
Function to extract the relevant pixel values from a raw frame and sort them into a usable array.
Relies on the GLOB_SC_CAL struct for where the relevant pixels are.
Inputs: 
    data - raw science camera frame
    O - array to save data to. Consists of a 20x3 matrix, with the rows being each of the 2 polarisations and 10
        wavelengths, and the 3 columns being the 3 tricoupler outputs.
Outputs:
    Saves the data to the matrix O
    Also saves total flux to GLOB_SC_TOTAL_FLUX
*/
void extractToMatrix(unsigned short* data, Eigen::Matrix<double, 20, 3> & O);

/*
Function to set the reference pixel positions.
WAVELENGTHS: 0.6063 0.6186 0.6316 0.6454 0.66 0.6755 0.6918 0.7092 0.7277 0.7473
Inputs:
    xref - location of the anchoring wavelength.
           Currently, this is the location of for 630nm, offset of -7 for red end on the left, or +2 if on the right
           for 660nm, set the xref offset to be -5 if red end on the left, or +4 if on the right
    yref - location of first row
Outputs:
    Saves all positions to the GLOB_SC_CAL struct
*/
int setPixelPositions(int xref, int yref);

/*
Alternative functions when fringe scanning using the FFT method (rather than SNR method)

extern double GLOB_SC_SCAN_FFT_LS[6][6];
int init_fringe_scan();
int fringeScan(unsigned short* data);
int fringeScanAndSave(unsigned short* data);
*/
#endif // _SC_SETUP_
