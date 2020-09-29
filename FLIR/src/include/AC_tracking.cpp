#include <vector>
#include <iostream>
#include <cmath>
#include "FLIRCamera.h"
#include "fringeLock_vis.h"
#include "ZaberActuator.h"
#include "helperFunc.h"
#include <zaber/motion/binary.h>

using namespace std;
using namespace zaber::motion;

/* FUNCTIONS TO PERFORM FRINGE LOCKING WITH
   NOTE: THERE IS SOME VERSION OF A RACE CONDITION IN HERE. NEED TO CHECK IF
   THIS IS A CONCERN...
*/

static int num_delays, num_channels;
static int outputA_idx, outputB_idx;
static std::vector<double> trial_delays;
static std::vector<std::vector<double>> trial_fringes;


double sellmeier(double lam){

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

double group_index(double lam){

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


double phaseshift_glass(double lam, double len, double lam0){

    double n = sellmeier(lam*1e6);
    double n_grp = group_index(lam0*1e6);
    double OPD = (n-n_grp)*len;
    double phase_shift = OPD*2*kPi/lam;

    return phase_shift;

}


/* High level function to perform fringe locking with visibilities
   INPUTS:
      Fcam - FLIRCamera class
      stage - ZaberActuator class
      fringe_config - table of configuration values for the fringe configuration
*/
void CalcTrialFringes(FLIRCamera Fcam, ZaberActuator stage, toml::table fringe_config){

    num_channels = fringe_config["positions"]["num_channels"].value_or(0);
    outputA_idx = fringe_config["positions"]["A"]["indices"][0].value_or(0);
    outputB_idx = fringe_config["positions"]["B"]["indices"][0].value_or(0);
    double scale_delay = fringe_config["tracking"]["scale_delay"].value_or(0.0);
    num_delays = fringe_config["tracking"]["num_delays"].value_or(0);
    toml::array wavelengths = *fringe_config.get("positions.wavelengths")->as_array();
    double bandpass = wavelengths[1] - wavelengths[0];
    double wavenumber_bandpass = 1/wavelengths[0] - 1/wavelengths[num_channels];

    double disp_length = fringe_config["tracking"]["AC"]["disp_length"].value_or(0.0);
    double disp_lam0 = fringe_config["tracking"]["AC"]["disp_lam0"].value_or(0.0);

    trial_delays = arange<double>(-num_delays/2 + 1,num_delays/2);

    for (int i = 0;i<num_delays;i++){
        trial_delays[i] *= scale_delay/wavenumber_bandpass;
    }

    trial_fringes.resize(num_delays);
    for (int i =0;i<num_delays;i++){
        trial_fringes[i].resize(num_channels);
    }

    for (int i = 0;i<num_delays;i++){
        for (int j = 0;j<num_channels;j++) {
            double envelope = sinc(trial_delays[i]*bandpass/(wavelengths[j]*wavelengths[j]));
            double sinusoid = cos(2*kPi*trial_delays[i]/wavelengths[j] - phaseshift_glass(wavelengths[j],disp_length,disp_lam0));
            trial_fringes[i][j] = envelope*sinusoid;
        }
    }

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

/* Callback function to be called each time a new frame is taken by the camera during
   fringe locking. Calls FringeLock on a number of pixels, plots the data and checks
   if the SNR is good enough to claim we have found fringes.

   INPUTS:
      frame - Image array data

   OUTPUTS:
      0 if fringes have been found
      1 otherwise
*/
int VisCalc(unsigned short * frame){

    double gamma_r [num_channels];
    double std_gamma_r [num_channels];
    double gamma_sum=0;

    for (int i=0; i<num_channels;i++){
        double outputA=(double)frame[outputA_idx+i];
        double outputB=(double)frame[outputB_idx+i];

        gamma_r [i] = outputA - outputB;
        std_gamma_r [i] = sqrt(outputA + outputB);
        gamma_sum += abs(gamma_r[i]);
    }

    double min_chi2 = 10000;
    int min_chi2_idx = 0;

    for (int i = 0;i<num_delays;i++){
        double chi2 = 0;

        for (int j = 0;j<num_channels;j++) {
            double num = (trial_fringes[i][j] - gamma_r[j]/gamma_sum);
            double den = (std_gamma_r[j]/gamma_sum);
            chi2 += num*num/(den*den);
        }

        if (chi2 < min_chi2){
            min_chi2 = chi2;
            min_chi2_idx = i;
        }
    }

    double found_delay = trial_delays[min_chi2_idx];

    //DO SOMETHING HERE WITH DELAY!!
    cout << "I found the delay to be: " << found_delay << endl;

    return 1;
}
