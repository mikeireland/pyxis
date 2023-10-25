// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>
#include <iostream>
#include "setup.hpp"
#include "globals.h"

// Declare arrays
Eigen::MatrixXcd GLOB_SC_DELAYMAT;
Eigen::MatrixXd GLOB_SC_DELAY_FOREGROUND_AMP;
Eigen::MatrixXd GLOB_SC_DELAY_AVE;
Eigen::MatrixXd GLOB_SC_V2;

// Set initial values
int GLOB_SC_WINDOW_INDEX = 0;
double GLOB_SC_WINDOW_ALPHA = 1.0;
double GLOB_SC_GD = 0.0;
double GLOB_SC_V2SNR = 0.0;

// Current delays
Eigen::ArrayXcd delays;

// Functions to take the median of an array 
template<typename Derived>
typename Derived::Scalar median( Eigen::DenseBase<Derived>& d ){
    auto r { d.reshaped() };
    std::sort( r.begin(), r.end() );
    return r.size() % 2 == 0 ?
        r.segment( (r.size()-2)/2, 2 ).mean() :
        r( r.size()/2 );
}

template<typename Derived>
typename Derived::Scalar median( const Eigen::DenseBase<Derived>& d ){
    typename Derived::PlainObject m { d.replicate(1,1) };
    return median(m);
}

/* 
Calculates the matrix of all trial delays vs wavelengths (10) and polarisations (2)
Inputs:
    numDelays - How many trial delays to use?
    delaySize - What is the spacing between delays (in um)?
Output:
    Saves the trial delays in GLOB_SC_DELAYMAT, with the first 10 rows being pol 1, 
    and the second 10 rows being pol 2
*/
int calcTrialDelayMat(int numDelays, double delaySize){

    // Initialise a phasor matrix, of size numDelays x 20
    Eigen::MatrixXcd phasors = Eigen::MatrixXcd::Zero(numDelays,20);

    double edge = static_cast<double>(numDelays)*delaySize*0.5;

    delays = Eigen::ArrayXcd::LinSpaced(numDelays, -edge, edge);

    for(int k=0;k<numDelays;k++){
        for(int l=0;l<10;l++){
            Cd num = 2*kPi*I*delays(k)/GLOB_SC_CAL.wavelengths[l];
            phasors(k,l) = num;
            phasors(k,l+10) = num;
        }
    }

    GLOB_SC_DELAYMAT = phasors.array().exp();

    return 0;

}

/* 
Main function to take in a frame and calculate the fringe envelope
Inputs:
    data - Raw frame from the science camera
    fringe_envelope - array to save the fringe envelope
Output:
    Saves the calculated envelope in fringe_envelope
    Also saves an estimate of the V2 in GLOB_SC_V2
*/
int calcFringeEnvelope(unsigned short* data, Eigen::MatrixXcd& fringe_envelope) {
    
    Eigen::Matrix<double, 20, 3> O; // Raw output data: three outputs for each pol/wavelength
    Eigen::Matrix<Cd, 20, 3> V; // Visibilities [Real(V), Imaginary(V), Flux(V)] for each pol/wavelength
    Eigen::Matrix<Cd, 20, 1> g; // Complex coherence vector
    Eigen::Matrix<Cd, 20, 1> SNR; // SNR vector
    
    extractToMatrix(data,O); // Extract relevant pixels

    // Convert to V(isibilities) via P2VM
    for(int k=0;k<20;k++){
        Eigen::Matrix<Cd, 3, 1> O_i = O.row(k);
        V.row(k) = GLOB_SC_P2VM_l[k]*O_i;
    }

    // Get complex coherence vector (g) as a function of wavelength
    g = (V.col(0) + I*V.col(1));
    g = g.array()*((V.col(2)).array().inverse());

    // Calculate V2
    GLOB_SC_V2 = g.array().abs2().real();

    // Save fringe envelope
    fringe_envelope = GLOB_SC_DELAYMAT*g;
    return 0;
}

/*
Function to estimate the foreground fringe envelope of the science camera data (i.e 
taking frames where there is injection, but no fringes)
Inputs:
    data - raw frame
Output:
    Saves calculated foreground amplitude in GLOB_SC_DELAY_FOREGROUND_AMP
*/
int calcForeground(unsigned short* data){

    Eigen::MatrixXcd fringe_envelope; // Complex foreground envelope
    calcFringeEnvelope(data, fringe_envelope);

    // Take the amplitude of the complex foreground envelope
    GLOB_SC_DELAY_FOREGROUND_AMP = fringe_envelope.cwiseAbs2().real();

    return 0;
}

/*
Main function to estimate the group delay from a frame of the science camera
Inputs:
    data - raw science camera frame
Outputs
    Saves relevant fringe envelope averages in GLOB_SC_DELAY_AVE, 
    estimated V2 SNR in GLOB_SC_V2SNR,
    and estimated group delay in GLOB_SC_GD
*/
int calcGroupDelay(unsigned short* data){

    // Calculate the fringe envelope
    Eigen::MatrixXcd fringe_envelope; 
    calcFringeEnvelope(data, fringe_envelope);

    // Estimate the noise using the median values of the real and imaginary amplitudes
    Eigen::ArrayXd envelope_real, envelope_im;
    envelope_real = fringe_envelope.real().cwiseAbs2();
    envelope_im = fringe_envelope.imag().cwiseAbs2();
    double noise_med_real = median(envelope_real); // Take the median
    double noise_med_im = median(envelope_im); // Take the median
    double noise = sqrt(noise_med_real*noise_med_real + noise_med_im*noise_med_im); // Average

    // Remove the foreground amplitude from the current fringe envelope
    Eigen::MatrixXd delay_current_amp = fringe_envelope.cwiseAbs2().real() - GLOB_SC_DELAY_FOREGROUND_AMP;
    
    //Moving average/fading memory
    if (not GLOB_SC_WINDOW_INDEX){ // Check if it is the first frame
        GLOB_SC_DELAY_AVE = delay_current_amp;
        GLOB_SC_WINDOW_INDEX = 1;
    } else{
        // Fading memory 
        GLOB_SC_DELAY_AVE = GLOB_SC_WINDOW_ALPHA*delay_current_amp + (1.0-GLOB_SC_WINDOW_ALPHA)*GLOB_SC_DELAY_AVE;
    }

    // Find the maximum of the fringe envelope
    Eigen::Index maxRow, maxCol;
    double maxAmp = GLOB_SC_DELAY_AVE.maxCoeff(&maxRow,&maxCol);
    
    // Extract the V2 SNR from this fringe envelope maximum and the noise
    GLOB_SC_V2SNR = abs(maxAmp)/noise;

    // Estimate group delay
    GLOB_SC_GD = delays(maxRow,maxCol).real();

    return 0;
}
