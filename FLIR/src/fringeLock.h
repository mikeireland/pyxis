#ifndef _FRINGELOCK_
#define _FRINGELOCK_

#include <vector>
#include "FLIRCamera.h"

// Template function to replicate the np.arange function in Python
template<typename T>
std::vector<T> arange(T start, T stop, T step = 1) {
    std::vector<T> values;
    for (T value = start; value < stop; value += step)
        values.push_back(value);
    return values;
};

// Struct to hold the fringe locking data required for each channel
struct fringe_lock_data{
    // Vector of fluxes for that pixel in each frame
    std::vector<unsigned short> flux;
    // Index of the pixel within the frame
    int flux_idx;
    // Central wavenumber of the channel
    double wavenumber;
    // Index for the FFT frequency that this channel should peak at
    int fft_signal_idx;
};

/* High level function to perform fringe locking
   INPUTS:
      Fcam - Camera class
*/
void FringeLock(FLIRCamera Fcam);


/* Low level function to take a pixel from the detector, perform an
   FFT on its past data and calculate the SNR on the frequency it should
   peak at if there are fringes

   INPUTS:
      frame - Image array data
      fringe_lock_data - struct containing indices and past data for the desired pixel
   
   OUTPUTS:
      SNR of the peak frequency of the FFT
*/
double FringeFFT(unsigned short * frame, struct fringe_lock_data flux_data);


/* Callback function to be called each time a new frame is taken by the camera during
   fringe locking. Calls FringeLock on a number of pixels, plots the data and checks
   if the SNR is good enough to claim we have found fringes.

   INPUTS:
      frame - Image array data
   
   OUTPUTS:
      0 if fringes have been found
      1 otherwise
*/
int FringeScan(unsigned short * frame);

#endif // _FRINGELOCK_
