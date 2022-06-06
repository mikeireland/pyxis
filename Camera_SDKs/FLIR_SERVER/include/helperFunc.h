#ifndef _HELPERFUNC_
#define _HELPERFUNC_

#include <string>
#include <pthread.h>
#include <vector>
extern const double kPi;

//Flags
extern int GLOB_CAM_STATUS;
extern int GLOB_RECONFIGURE;
extern int GLOB_RUNNING;
extern int GLOB_STOPPING;
extern int GLOB_NUMFRAMES;

//config_file
extern char * GLOB_CONFIGFILE ;

//Thread
extern pthread_t camthread;

//Locks
extern pthread_mutex_t flag_lock;
extern pthread_mutex_t img_array_lock;

struct configuration{
    int gain; 
    int exptime; 
    int width; 
    int height; 
    int offsetX; 
    int offsetY; 
    int blacklevel;
    int buffersize;
    std::string savedir;
};


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

#endif // _HELPERFUNC_
