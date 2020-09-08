#include <vector>
#include <iostream>
#include <complex>
#include <fftw3.h>
#include <cmath>
#include "FLIRCamera.h"
#include "fringeLock.h"

static int lock_SNR;
static int signal_size;
static int window_size;

static struct fringe_lock_data flux_data_ls[3];

static fftw_plan plan;
static double *in;
static fftw_complex *out;


void FringeLock(FLIRCamera Fcam){
    int old_exposure = Fcam.exposure_time;

    int scan_rate = Fcam.config["fringe"]["locking"]["scan_rate"].value_or(0);
    lock_SNR = Fcam.config["fringe"]["locking"]["lock_SNR"].value_or(0);
    signal_size = Fcam.config["fringe"]["locking"]["signal_size"].value_or(0);
    double scan_width = Fcam.config["fringe"]["locking"]["scan_width"].value_or(0);
    Fcam.exposure_time = Fcam.config["fringe"]["locking"]["exposure_time"].value_or(0);
    window_size = Fcam.config["fringe"]["locking"]["window_size"].value_or(0);

    double meters_per_frame = scan_rate*Fcam.exposure_time;

    std::vector<double> trial_delays = arange<double>(-scan_width/2, scan_width/2,step = meters_per_frame);

    int num_frames = scan_width/meters_per_frame;

    std::vector<double> values = arange<double>(0,window_size/2+1);
    double period = window_size*meters_per_frame;

    for (int i=0;i<3;i++){
       std::string output = Fcam.config["fringe"]["locking"]["selections"][i][0].value_or('');
       int channel = Fcam.config["fringe"]["locking"]["selections"][i][1].value_or(0);
       flux_data_ls[i].flux_idx = Fcam.config["fringe"]["positions"][output]["indices"][channel].value_or(0);
       double wavelength = Fcam.config["fringe"]["positions"][output]["frequencies"][channel].value_or(0);
       double wavenumber = 1/wavelength;
       flux_data_ls[i].wavenumber = wavenumber;
       flux_data_ls[i].flux (window_size,0);

       double min_residual = 100;
       int min_arg;
       for(int j=0; j<(window_size/2+1); j++){
          double residual = values[j] - wavenumber;
          if (residual < min_residual){
             min_residual = min_residual;
             min_arg = j;
          }
       }

       flux_data_ls[i].fft_signal_idx = min_arg;
    }

    in = (double*) fftw_malloc(sizeof(double) * window_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (window_size/2+1));
    plan = fftw_plan_dft_r2c_1d(window_size, in, out, FFTW_MEASURE);

    // INITIALISE ACTUATOR

    // Allocate memory for the image data (given by size of image and buffer size)
    unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

    // Start Frame capture
    Fcam.GrabFrames(num_frames, image_array, FringeScan);

    Fcam.exposure_time = old_exposure;

    // Retrieve Delay and print

}

double FringeFFT(unsigned short * frame, struct fringe_lock_data flux_data){

    flux_data.flux.push_back(frame[flux_data.flux_idx]);

    std::copy(flux_data.end()-window_size, flux_data.end(), in);

    fftw_execute(plan);

    double data, mean, signal = 0, std = 0;


    for (int i=1;i<(window_size/2+1);i++){
        data = abs(out[i]);

        if (abs(i-flux_signal_idx) < signal_size){
            if (data > signal){
                signal = data;
            }
        }
        else {
            sum += data;
        }
    }

    mean = sum/(window_size/2 - (2*signal_size-1));

    for (int i=1;i<(window_size/2+1);i++){
        if (abs(i-flux_signal_idx) > signal_size){
            data = abs(out[i]);
            std += pow(data - mean, 2);
        }
    }

    std = sqrt(std/(window_size/2 - (2*signal_size-1)));

    return signal/std;
}

int FringeScan(unsigned short * frame){

    double SNR_1 = FringeFFT(frame, flux_data_ls[0]);
    double SNR_2 = FringeFFT(frame, flux_data_ls[1]);
    double SNR_3 = FringeFFT(frame, flux_data_ls[2]);

    //GNU PLOT

    if ((SNR_1 > lock_SNR) && (SNR_2 > lock_SNR) && (SNR_3 > lock_SNR)){
        return 0;
    }

    // MOVE ACTUATOR

    return 1;
}
