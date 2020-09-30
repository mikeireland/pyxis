#ifndef _ACLENGTH_
#define _ACLENGTH_

#include "toml.hpp"

// Functions to determine the extra dispersion length of an AC Coupler??

/* Calculates a bunch of trial fringes with different lengths fro the function
    "findLength"
    INPUTS
        fringe_config - table of configuration values for the fringe configuration
*/
void CalcTrialLengths(toml::table fringe_config);


/* Estimates the added length on the coupler. Used as a callback function from the
   camera.
    INPUTS
        frame - frame_data from the FLIR camera
    OUTPUTS:
       0 if fringes have been found
       1 otherwise
*/
int findLength(unsigned short * frame);

#endif // _ACLENGTH_
