#include <vector>
#include <iostream>
#include <complex>
#include <fftw3.h>
#include <cmath>
#include "FLIRCamera.h"
#include "fringeLock.h"
#include "ZaberActuator.h"
#include "helperFunc.h"
#include <zaber/motion/binary.h>

using namespace std;
using namespace zaber::motion;

/* FUNCTIONS TO PERFORM FRINGE LOCKING WITH */

static double lock_SNR;
static int signal_size;
static int window_size;

static struct fringe_lock_data flux_data_ls[3];

static fftw_plan plan;
static double *in;
static fftw_complex *out;


/* High level function to perform fringe locking
   INPUTS:
      Fcam - Camera class
*/
void FringeLock(FLIRCamera Fcam, ZaberActuator stage, toml::table fringe_config){

    // Keep old exposure time to give back later
    int old_exposure = Fcam.exposure_time;

    // Read from config file

    // How fast to move the actuator during the scan in um/s
    double scan_rate = fringe_config["locking"]["scan_rate"].value_or(0.0);
    // SNR to trigger fringes being "found"
    lock_SNR = fringe_config["locking"]["lock_SNR"].value_or(0.0);
    // How wide (in FFT frequency units) should the signal be defined as?
    signal_size = fringe_config["locking"]["signal_size"].value_or(0);
    // Over what distance should we scan?
    double scan_width = fringe_config["locking"]["scan_width"].value_or(0.0);
    // Exposure time of camera during scan
    Fcam.exposure_time = fringe_config["locking"]["exposure_time"].value_or(0);
    // Window size (in frames) to perform the FFT over
    window_size = fringe_config["locking"]["window_size"].value_or(0);

    // Number of meters per frame the stage will scan
    double meters_per_frame = scan_rate*(Fcam.exposure_time/1e6);

    // List of trial delays to be scanned
    std::vector<double> trial_delays = arange<double>(-scan_width/2, scan_width/2,meters_per_frame);

    // Maximum number of frames to take over the scan
    int num_frames = scan_width/meters_per_frame;

    // Frequencies for the resultant FFT
    std::vector<double> values = arange<double>(0,window_size/2+1);
    double period = window_size*meters_per_frame;

    // Loop over the pixels we are looking at
    for (int i=0;i<3;i++){
       // What channel are we viewing?
       std::string output = fringe_config["locking"]["selections"][i][0].value_or("");
       int channel = fringe_config["locking"]["selections"][i][1].value_or(0);

       // Find the wavenumber and index in the image array of the desired pixel
       flux_data_ls[i].flux_idx = fringe_config["positions"][output]["indices"][channel].value_or(0);
       double wavelength =fringe_config["positions"][output]["frequencies"][channel].value_or(0);
       double wavenumber = 1/wavelength;
       flux_data_ls[i].wavenumber = wavenumber;

       // Initially fill the flux history array with zeros
       flux_data_ls[i].flux.assign(window_size,0);

       // Identify the index of the frequency that should peak if we have fringes
       double min_residual = 100;
       int min_arg;
       for(int j=0; j<(window_size/2+1); j++){
          double residual = values[j]/period - wavenumber;
          if (residual < min_residual){
             min_residual = min_residual;
             min_arg = j;
          }
       }

       flux_data_ls[i].fft_signal_idx = min_arg;
    }

    // Initialise FFTW arrays and FFTW plan
    in = (double*) fftw_malloc(sizeof(double) * window_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (window_size/2+1));
    plan = fftw_plan_dft_r2c_1d(window_size, in, out, FFTW_MEASURE);

    // Actuator Starting position (wait for max 100s until arrived)
    double start_pos = fringe_config["locking"]["start_pos"].value_or(0.0);
    stage.MoveAbsolute(start_pos, Units::LENGTH_MILLIMETRES, 100);

    // Allocate memory for the image data (given by size of image and buffer size)
    unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

    // Init camera
    Fcam.InitCamera();

    // Start actuator movement
    stage.MoveAtVelocity(scan_rate, Units::VELOCITY_METRES_PER_SECOND);

    // Start frame capture and scan
    Fcam.GrabFrames(num_frames, image_array, FringeScan);

    // Stop actuator movement
    stage.Stop();

    // Deinit camera
    Fcam.DeinitCamera();

    // Return old exposure time
    Fcam.exposure_time = old_exposure;

    // Retrieve Delay and print
    cout << endl << "END LOCKING "<< endl;

    // Memory Management and reset
    free(image_array);
    fftw_destroy_plan(plan);
    fftw_free(in); fftw_free(out);
}


/* Low level function to take a pixel from the detector, perform an
   FFT on its past data and calculate the SNR on the frequency it should
   peak at if there are fringes

   INPUTS:
      frame - Image array data
      fringe_lock_data - struct containing indices and past data for the desired pixel

   OUTPUTS:
      SNR of the peak frequency of the FFT
*/
double FringeFFT(unsigned short * frame, struct fringe_lock_data *flux_data){

    // Add current flux from pixel into the array
    (*flux_data).flux.push_back(frame[(*flux_data).flux_idx]);

    // Copy the last "window_size" number of flux values into the FFT vector
    std::copy((*flux_data).flux.end()-window_size, (*flux_data).flux.end(), in);

    // Perform the FFT
    fftw_execute(plan);

    double data, mean, sum=0., signal = 0., std = 0.;
    std::complex<double> temp;

    // Loop through the FFT to calculate SNR
    for (int i=1;i<(window_size/2+1);i++){
        temp = std::complex<double>(out[i][0], out[i][1]);
        data = std::abs( std::pow(temp, 2) );

        // Find the maximum peak of the FFT within the "signal" window
        if (abs(i-(*flux_data).fft_signal_idx) < signal_size){
            if (data > signal){
                signal = data;
            }
        }
        // Otherwise, add all the "noisy" frequencies
        else {
            sum += data;
        }
    }

    // Find the mean of the noisy values
    mean = sum/(window_size/2 - (2*signal_size-1));

    // Calculate standard deviation of the noise
    for (int i=1;i<(window_size/2+1);i++){
        if (abs(i-(*flux_data).fft_signal_idx) > signal_size){
            data = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
            std += pow(data - mean, 2);
        }
    }

    std = sqrt(std/(window_size/2 - (2*signal_size-1)));

    // Return SNR
    return signal/std;
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
int FringeScan(unsigned short * frame){

    // Run the FFT routine on three different pixels/channels
    double SNR_1 = FringeFFT(frame, &flux_data_ls[0]);
    double SNR_2 = FringeFFT(frame, &flux_data_ls[1]);
    double SNR_3 = FringeFFT(frame, &flux_data_ls[2]);

    cout << "FINISHED MY FFTS with SNRs: " << SNR_1 << ", "<< SNR_2 << ", " << SNR_3 << endl;

    // If all are above the desired SNR, end the acquisition there
    if ((SNR_1 > lock_SNR) && (SNR_2 > lock_SNR) && (SNR_3 > lock_SNR)){
        return 0;
    }

    return 1;
}
