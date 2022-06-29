#ifndef _GLOBALS_
#define _GLOBALS_

#include <string>
#include <pthread.h>
#include <vector>
extern const double kPi;

//Flags
extern int GLOB_CAM_STATUS;
extern int GLOB_RECONFIGURE;
extern int GLOB_RUNNING;
extern int GLOB_STOPPING;

//Global Params
extern int GLOB_NUMFRAMES;
extern int GLOB_IMSIZE;
extern int GLOB_WIDTH;

//config_file
extern char * GLOB_CONFIGFILE ;

//Thread
extern pthread_t GLOB_CAMTHREAD;

//Locks
extern pthread_mutex_t GLOB_FLAG_LOCK;
extern pthread_mutex_t GLOB_LATEST_FILE_LOCK;
extern pthread_mutex_t GLOB_LATEST_IMG_INDEX_LOCK;
extern pthread_mutex_t *GLOB_IMG_MUTEX_ARRAY;

// Image Array
extern unsigned short *GLOB_IMG_ARRAY;

// Latest file/image
extern std::string GLOB_LATEST_FILE ;
extern int GLOB_LATEST_IMG_INDEX;

struct configuration{
    float gain; 
    float exptime; 
    int width; 
    int height; 
    int offsetX; 
    int offsetY; 
    float blacklevel;
    int buffersize;
    std::string savedir;
};

extern configuration GLOB_CONFIG_PARAMS;

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

#endif // _GLOBALS_
