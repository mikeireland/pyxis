#include <vector>
#include <iostream>
#include <cmath>
#include "AC_determine_length.h"
#include "AC_tracking.h"
#include "helperFunc.h"
#include "toml.hpp"

using namespace std;

static int num_lengths, num_channels;
static int outputA_idx, outputB_idx;
static std::vector<double> trial_lengths;
static std::vector<std::vector<double>> trial_fringes;

/* Calculates a bunch of trial fringes with different lengths fro the function
    "findLength"
    INPUTS
        fringe_config - table of configuration values for the fringe configuration
*/
void CalcTrialLengths(toml::table fringe_config){

    // Number of wavelength channels
    num_channels = fringe_config["positions"]["num_channels"].value_or(0);

    // Indices for the start of each output
    outputA_idx = fringe_config["positions"]["A"]["indices"][0].value_or(0);
    outputB_idx = fringe_config["positions"]["B"]["indices"][0].value_or(0);

    // Maximum extra length for the coupler
    double max_length = fringe_config["tracking"]["AC"]["max_length"].value_or(0.0);

    // Number of lengths to check
    num_lengths = fringe_config["tracking"]["AC"]["num_lengths"].value_or(0);

    // Wavelengths of each channel
    std::vector<double> wavelengths;
    for (int i=0; i<num_channels;i++){
        wavelengths.push_back(fringe_config["positions"]["wavelengths"][i].value_or(0.0));
    }

    // Wavelength and Wavenumber bandpass
    double bandpass = wavelengths[1] - wavelengths[0];

    // Central wavelength for group index
    double disp_lam0 = fringe_config["tracking"]["AC"]["disp_lam0"].value_or(0.0);

    // Delay to estimate length from (usually 0)
    double delay = fringe_config["tracking"]["AC"]["delay0"].value_or(0.0);

    // Trial lengths
    trial_lengths = arange<double>(-num_lengths/2 + 1,num_lengths/2);
    for (int i = 0;i<num_lengths;i++){
        trial_lengths[i] *= 2*max_length/(double)num_lengths;
    }


    // Resize trial fringe vector
    trial_fringes.resize(num_lengths);
    for (int i =0;i<num_lengths;i++){
        trial_fringes[i].resize(num_channels);
    }

    // Calculate trial fringes
    for (int i = 0;i<num_lengths;i++){
        for (int j = 0;j<num_channels;j++) {
            double envelope = sinc(delay*bandpass/(wavelengths[j]*wavelengths[j]));
            double sinusoid = cos(2*kPi*delay/wavelengths[j] - phaseshift_glass(wavelengths[j],trial_lengths[i],disp_lam0));

            trial_fringes[i][j] = envelope*sinusoid;
        }
    }



    // Normalise trial fringes over wavelength
    for (int i = 0;i<num_lengths;i++){
        double wavelength_sum = 0;
        for (int j = 0;j<num_channels;j++) {
            wavelength_sum += abs(trial_fringes[i][j]);
        }
        for (int j = 0;j<num_channels;j++) {
            trial_fringes[i][j] /= wavelength_sum;
        }
    }



    cout << "Done setting up lengths" << endl;
}


/* Estimates the added length on the coupler. Used as a callback function from the
   camera.
    INPUTS
        frame - frame_data from the FLIR camera
*/
int findLength(unsigned short * frame){

    double gamma_r [num_channels];
    double std_gamma_r [num_channels];
    double gamma_sum=0;

    // Calculate the real part of the coherence for each channel
    for (int i=0; i<num_channels;i++){
        // Flux outputs
        double outputA=(double)frame[outputA_idx+i];
        double outputB=(double)frame[outputB_idx+i];

        // Coherence, and calculate the sum over all wavelengths for normalisation
        gamma_r [i] = outputA - outputB;
        std_gamma_r [i] = sqrt(outputA + outputB);
        gamma_sum += abs(gamma_r[i]);
    }

    double min_chi2 = 10000000;
    int min_chi2_idx = 0;

    for (int i = 0;i<num_lengths;i++){

        double chi2 = 0;

        // Calculate Chi2 from the trial fringes
        for (int j = 0;j<num_channels;j++) {
            double num = (trial_fringes[i][j] - gamma_r[j]/gamma_sum);
            double den = (std_gamma_r[j]/gamma_sum);
            chi2 += num*num/(den*den);
        }

        // Is it the minimum?
        if (chi2 < min_chi2){
            min_chi2 = chi2;
            min_chi2_idx = i;
        }
    }

    // Return the estimated length
    double found_length = trial_lengths[min_chi2_idx];

    cout << "I found the length to be: " << found_length << endl;

    return 1;
}
