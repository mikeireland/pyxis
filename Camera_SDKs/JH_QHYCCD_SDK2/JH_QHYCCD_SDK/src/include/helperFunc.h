#ifndef _HELPERFUNC_
#define _HELPERFUNC_

extern const double kPi;

double sinc(double x);

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
std::string Label(std::string str, const size_t num = 25, const char padding_char = ' ');

void char_to_short(unsigned char* input, unsigned short* output, unsigned long length);


#endif // _HELPERFUNC_
