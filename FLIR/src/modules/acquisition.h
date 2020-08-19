// Based on the work done by https://github.com/miks/spinnaker-fps-test

#ifndef _ACQUISITION_
#define _ACQUISITION_

#include "Spinnaker.h"
#include "cpptoml/cpptoml.h"

/* A struct that holds time data for image acquisition
   ELEMENTS:
      total_exposure - total exposure time to take a number of frames
      timestamp - GMT time at which the exposure started

*/
struct Times{
    double total_exposure;
    char* timestamp;
};

/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUTS:
      Padded string

*/
std::string Label(std::string str, const size_t num = 20, const char padding_char = ' ');

/* Function to take a number of images with a camera and optionally work on them.
   INPUTS:
      pCam - Spinnaker pointer to a FLIR camera
      config - cpptoml pointer to a configuration table
      num_frames - number of images to take
      fits_array - allocated vector to store image data in
      times_struct - a structure that allows the storage of a timestamp and duration of the exposures
      use_func - a flag as to whether to apply a real time function to the image data
      *f - a function that will be applied to each image in real time if use_func is set

*/
void GrabFrames(Spinnaker::CameraPtr pCam, std::shared_ptr<cpptoml::table> config, unsigned long num_frames, unsigned short* fits_array, Times& times_struct, int use_func, void (*f)(unsigned short*));

#endif // _ACQUISITION_
