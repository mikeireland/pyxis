// Function to calculate trial delays

// Group delay calc function

#include <Eigen/Dense>
#include <iostream>
#include "setup.hpp"
#include "globals.h"

Eigen::MatrixXcd GLOB_SC_DELAYMAT;
Eigen::MatrixXd GLOB_SC_DELAY_FOREGROUND_AMP;
Eigen::MatrixXd GLOB_SC_DELAY_AVE;
Eigen::MatrixXd GLOB_SC_V2;

int GLOB_SC_WINDOW_INDEX = 0;
double GLOB_SC_WINDOW_ALPHA = 1.0;

double GLOB_SC_GD = 0.0;
double GLOB_SC_V2SNR = 0.0;

Eigen::ArrayXcd delays;

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

// Calculates the matrix of all trial delays vs wavelengths
int calcTrialDelayMat(int numDelays, double delaySize){

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

// Main function to take in a frame and calculate the group delay
int calcFringeEnvelope(unsigned short* data, Eigen::MatrixXcd& fringe_envelope) {
    
    Eigen::Matrix<double, 20, 3> O;
    Eigen::Matrix<Cd, 20, 3> V;
    Eigen::Matrix<Cd, 20, 1> g;
    Eigen::Matrix<Cd, 20, 1> SNR;
    
    extractToMatrix(data,O);

    // Convert to V(isibilities) via P2VM
    for(int k=0;k<20;k++){
        Eigen::Matrix<Cd, 3, 1> O_i = O.row(k);
        V.row(k) = GLOB_SC_P2VM_l[k]*O_i;
    }

    // Get complex coherence vector (g) as a function of wavelength
    g = (V.col(0) + I*V.col(1));
    g = g.array()*((V.col(2)).array().inverse());

    GLOB_SC_V2 = g.array().abs2().real();
    //SNR = GLOB_SC_V2.array()*((V.col(2)).array())/20;

    //GLOB_SC_V2SNR = SNR.norm();
    fringe_envelope = GLOB_SC_DELAYMAT*g;
    return 0;
}

int calcForeground(unsigned short* data){

    Eigen::MatrixXcd fringe_envelope;
    calcFringeEnvelope(data, fringe_envelope);

    // Fourier transform sampling by multiplying by trial delay matrix
    GLOB_SC_DELAY_FOREGROUND_AMP = fringe_envelope.cwiseAbs2().real();

    return 0;
}



int calcGroupDelay(unsigned short* data){

    Eigen::MatrixXcd fringe_envelope;
    calcFringeEnvelope(data, fringe_envelope);

    Eigen::ArrayXd envelope_real, envelope_im;
    envelope_real = fringe_envelope.real().cwiseAbs2();
    envelope_im = fringe_envelope.imag().cwiseAbs2();
    double noise_med_real = median(envelope_real);
    double noise_med_im = median(envelope_im);
    double noise = sqrt(noise_med_real*noise_med_real + noise_med_im*noise_med_im);

    // Fourier transform sampling by multiplying by trial delay matrix
    Eigen::MatrixXd delay_current_amp = fringe_envelope.cwiseAbs2().real() - GLOB_SC_DELAY_FOREGROUND_AMP;
    
    //Moving average/fading memory
    if (not GLOB_SC_WINDOW_INDEX){
        GLOB_SC_DELAY_AVE = delay_current_amp;
        GLOB_SC_WINDOW_INDEX = 1;
    } else{
        GLOB_SC_DELAY_AVE = GLOB_SC_WINDOW_ALPHA*delay_current_amp + (1.0-GLOB_SC_WINDOW_ALPHA)*GLOB_SC_DELAY_AVE;
    }

    // Extract group delay from maximum of the fourier transform
    Eigen::Index maxRow, maxCol;
    double maxAmp = GLOB_SC_DELAY_AVE.maxCoeff(&maxRow,&maxCol);
    
    GLOB_SC_V2SNR = abs(maxAmp)/noise;

    GLOB_SC_GD = delays(maxRow,maxCol).real();

    return 0;
}
