#ifndef _FRINGELOCK_
#define _FRINGELOCK_

#include <vector>
#include "FLIRCamera.h"

template<typename T>
std::vector<T> arange(T start, T stop, T step = 1) {
    std::vector<T> values;
    for (T value = start; value < stop; value += step)
        values.push_back(value);
    return values;
};

struct fringe_lock_data{
    std::vector<unsigned short> flux;
    int flux_idx;
    double wavenumber;
    int fft_signal_idx;
};

void FringeLock(FLIRCamera Fcam);
double FringeFFT(unsigned short * frame, struct fringe_lock_data flux_data);
int FringeScan(unsigned short * frame);

#endif // _FRINGELOCK_
