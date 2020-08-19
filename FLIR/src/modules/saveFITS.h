#ifndef _SAVEFITS_
#define _SAVEFITS_

#include "cpptoml/cpptoml.h"
#include "acquisition.h"

/* Write a given vector array of image data as a FITS file
   INPUTS:
      config - cpptoml pointer to a configuration table
      fits_array - vector of image data to write
      times_struct - a structure that allows the storage of a timestamp and duration of the exposures

*/
int SaveFITS(std::shared_ptr<cpptoml::table> config, unsigned short* fits_array, Times times_struct);

#endif // _SAVEFITS_
