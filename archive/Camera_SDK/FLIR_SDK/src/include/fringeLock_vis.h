#ifndef _FRINGELOCK_
#define _FRINGELOCK_

#include <vector>
#include "FLIRCamera.h"
#include "ZaberActuator.h"


/* High level function to perform fringe locking with visibilities
   INPUTS:
      Fcam - FLIRCamera class
      stage - ZaberActuator class
      fringe_config - table of configuration values for the fringe configuration
*/
void FringeLockVis(FLIRCamera Fcam, ZaberActuator stage, toml::table fringe_config);


/* Callback function to be called each time a new frame is taken by the camera during
   fringe locking. Calculates the V^2 over both outputs, and returns early if the
   V^2 is above a certain value.

   INPUTS:
      frame - Image array data

   OUTPUTS:
      0 if fringes have been found
      1 otherwise
*/
int VisCalc(unsigned short * frame);

#endif // _FRINGELOCK_
