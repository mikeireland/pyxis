#ifndef _ACTRACKING_
#define _ACTRACKING_

#include "toml.hpp"


/*Calculate the refractive index of BK7 glass at a given wavelength
    INPUTS:
        lam = wavelength (in microns)
    OUTPUTS:
        refractive index n
*/
double sellmeier(double lam);


/*Calculate the group index of BK7 glass at a given wavelength
    INPUTS:
        lam = wavelength (in microns)
    OUTPUTS:
        group refractive index n_group
*/
double group_index(double lam);


/* Calculate the phase shift due to dispersion from unequal path length of glass
    INPUTS:
        lam = wavelength to calculate shift for (in m)
        len = extra length of glass (in m)
        lam_0 = reference wavelength (in m)
    OUTPUTS:
        dispersional phase shift in radians
*/
double phaseshift_glass(double lam, double len, double lam0);


/* Calculates a bunch of trial fringes with different delays for the function
    "findDelay"
    INPUTS
        fringe_config - table of configuration values for the fringe configuration
*/
void CalcTrialFringes(toml::table fringe_config);


/* Estimates the fringe delay. Used as a callback function from the camera.
    INPUTS
        frame - frame_data from the FLIR camera
    OUTPUTS:
       0 if fringes have been found
       1 otherwise
*/
int findDelay(unsigned short * frame);

#endif // _ACTRACKING_
