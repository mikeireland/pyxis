#include <vector>
#include <iostream>
#include <complex>
#include <fftw3.h>
#include <cmath>
#include "FLIRCamera.h"

struct fringe_lock_data{
    std::vector<unsigned short> flux;
    int flux_idx;
    int fft_signal_idx;
}

template<typename T>
std::vector<T> arange(T start, T stop, T step = 1) {
    std::vector<T> values;
    for (T value = start; value < stop; value += step)
        values.push_back(value);
    return values;
}

static int lock_SNR;
static int signal_size;
static int window_size;

static struct fringe_lock_data flux1_data, flux2_data, flux3_data;

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

    values = arange<double>(0,window_size/2);
    period = ;
    wavenumbers = ;

    flux1_data.flux_idx = ;
    flux1_data.fft_signal_idx = ;

    flux2_data.flux_idx = ;
    flux2_data.fft_signal_idx = ;

    flux3_data.flux_idx = ;
    flux3_data.fft_signal_idx = ;


    in = (double*) fftw_malloc(sizeof(double) * window_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (window_size/2+1));
    plan = fftw_plan_dft_r2c_1d(window_size, in, out, FFTW_MEASURE);

    // INITIALISE ACTUATOR

    // Allocate memory for the image data (given by size of image and buffer size)
    unsigned short *image_array = (unsigned short*)malloc(sizeof(unsigned short)*Fcam.width*Fcam.height*Fcam.buffer_size);

    // Start Frame capture
    Fcam.GrabFrames(num_frames, image_array, FringeScan)

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

    return signal/std

}

int FringeScan(unsigned short * frame){

    double SNR_1 = FringeFFT(frame, flux1_data);
    double SNR_2 = FringeFFT(frame, flux2_data);
    double SNR_3 = FringeFFT(frame, flux3_data);

    //GNU PLOT

    if ( < lock_SNR):{
        return 0;
    }

    // MOVE ACTUATOR

    return 1;
}
