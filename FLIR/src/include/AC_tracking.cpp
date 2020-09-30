#include <vector>
#include <iostream>
#include <cmath>
#include "AC_tracking.h"
#include "helperFunc.h"
#include "toml.hpp"

using namespace std;

static int num_delays, num_channels;
static int outputA_idx, outputB_idx;
static std::vector<double> trial_delays;
static std::vector<std::vector<double>> trial_fringes;


/*Calculate the refractive index of BK7 glass at a given wavelength
    INPUTS:
        lam = wavelength (in microns)
    OUTPUTS:
        refractive index n
*/
double sellmeier(double lam){

    // Sellmeier coefficients (Change for different glass)
    double B1, B2, B3, C1, C2, C3;
    B1 = 1.03961212;
    B2 = 0.231792344;
    B3 = 1.01046945;
    C1 = 6.00069867e-3;
    C2 = 2.00179144e-2;
    C3 = 103.560653;

    double lam2 = lam*lam;

    double n2 = 1 + B1*lam2/(lam2-C1) + B2*lam2/(lam2-C2) + B3*lam2/(lam2-C3);

    return sqrt(n2);
}


/*Calculate the group index of BK7 glass at a given wavelength
    INPUTS:
        lam = wavelength (in microns)
    OUTPUTS:
        group refractive index n_group
*/
double group_index(double lam){

    // Sellmeier coefficients (Change for different glass)
    double B1, B2, B3, C1, C2, C3;
    B1 = 1.03961212;
    B2 = 0.231792344;
    B3 = 1.01046945;
    C1 = 6.00069867e-3;
    C2 = 2.00179144e-2;
    C3 = 103.560653;

    double lam2 = lam*lam;

    double n = sellmeier(lam);

    double a1, a2, a3, a4, a5, a6;
    a1 = -B1*lam2/((lam2-C1)*(lam2-C1));
    a2 = B1/(lam2-C1);
    a3 = -B2*lam2/((lam2-C2)*(lam2-C2));
    a4 = B2/(lam2-C2);
    a5 = -B3*lam2/((lam2-C3)*(lam2-C3));
    a6 = B3/(lam2-C3);

    double adj_factor = lam2*(a1+a2+a3+a4+a5+a6)/n;

    return sqrt(n - adj_factor);
}


/* Calculate the phase shift due to dispersion from unequal path length of glass
    INPUTS:
        lam = wavelength to calculate shift for (in m)
        len = extra length of glass (in m)
        lam_0 = reference wavelength (in m)
    OUTPUTS:
        dispersional phase shift in radians
*/
double phaseshift_glass(double lam, double len, double lam0){

    double n = sellmeier(lam*1e6);
    double n_grp = group_index(lam0*1e6);
    double OPD = (n-n_grp)*len;
    double phase_shift = OPD*2*kPi/lam;

    return phase_shift;
}


/* Calculates a bunch of trial fringes with different delays for the function
    "findDelay"
    INPUTS
        fringe_config - table of configuration values for the fringe configuration
*/
void CalcTrialFringes(toml::table fringe_config){

    // Number of wavelength channels
    num_channels = fringe_config["positions"]["num_channels"].value_or(0);

    // Indices for the start of each output
    outputA_idx = fringe_config["positions"]["A"]["indices"][0].value_or(0);
    outputB_idx = fringe_config["positions"]["B"]["indices"][0].value_or(0);

    // Number to scale the trial delays by (smaller = finer)
    double scale_delay = fringe_config["tracking"]["scale_delay"].value_or(0.0);

    // Number of delays to test
    num_delays = fringe_config["tracking"]["num_delays"].value_or(0);

    // Wavelengths of each channel
    toml::array wavelengths = *fringe_config.get("positions.wavelengths")->as_array();

    // Wavelength and Wavenumber bandpass
    double bandpass = wavelengths[1] - wavelengths[0];
    double wavenumber_bandpass = 1/wavelengths[0] - 1/wavelengths[num_channels];

    // Extra length of the coupler for dispersion
    double disp_length = fringe_config["tracking"]["AC"]["disp_length"].value_or(0.0);
    // Central wavelength for group index
    double disp_lam0 = fringe_config["tracking"]["AC"]["disp_lam0"].value_or(0.0);

    // Trial delays
    trial_delays = arange<double>(-num_delays/2 + 1,num_delays/2);
    for (int i = 0;i<num_delays;i++){
        trial_delays[i] *= scale_delay/wavenumber_bandpass;
    }

    // Resize trial fringe vector
    trial_fringes.resize(num_delays);
    for (int i =0;i<num_delays;i++){
        trial_fringes[i].resize(num_channels);
    }

    // Calculate trial fringes
    for (int i = 0;i<num_delays;i++){
        for (int j = 0;j<num_channels;j++) {
            double envelope = sinc(trial_delays[i]*bandpass/(wavelengths[j]*wavelengths[j]));
            double sinusoid = cos(2*kPi*trial_delays[i]/wavelengths[j] - phaseshift_glass(wavelengths[j],disp_length,disp_lam0));
            trial_fringes[i][j] = envelope*sinusoid;
        }
    }

    // Normalise trial fringes over wavelength
    for (int i = 0;i<num_delays;i++){
        double wavelength_sum = 0;
        for (int j = 0;j<num_channels;j++) {
            wavelength_sum += abs(trial_fringes[i][j]);
        }
        for (int j = 0;j<num_channels;j++) {
            trial_fringes[i][j] /= wavelength_sum;
        }
    }
}


/* Estimates the fringe delay. Used as a callback function from the camera.
    INPUTS
        frame - frame_data from the FLIR camera
    OUTPUTS:
       0 if fringes have been found
       1 otherwise
*/
int findDelay(unsigned short * frame){

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

    double min_chi2 = 10000;
    int min_chi2_idx = 0;

    for (int i = 0;i<num_delays;i++){
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

    // Return the estimated delay
    double found_delay = trial_delays[min_chi2_idx];

    //DO SOMETHING HERE WITH DELAY!!
    cout << "I found the delay to be: " << found_delay << endl;

    return 1;
}
