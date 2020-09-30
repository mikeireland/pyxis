#include <cmath>
// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _HELPERFUNC_
#define _HELPERFUNC_

extern const double kPi = 3.141592654;

double sinc(double x){
    double result = sin(kPi*x)/(kPi*x);
    return result;
}

// Template function to replicate the np.arange function in Python
template<typename T>
std::vector<T> arange(T start, T stop, T step = 1) {
    std::vector<T> values;
    for (T value = start; value < stop; value += step)
        values.push_back(value);
    return values;
};

/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num = 25, const char padding_char = ' ') {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), padding_char);
        }
        return str + ": ";
    }

#endif // _HELPERFUNC_
