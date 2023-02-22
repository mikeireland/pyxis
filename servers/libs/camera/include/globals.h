#ifndef _GLOBALS_
#define _GLOBALS_

#include <string>
#include <pthread.h>
#include <vector>
extern const double kPi; //Pi constant

//Flags for server to communicate status.
extern int GLOB_CAM_STATUS; // Overall camera status
extern int GLOB_RECONFIGURE; // Do I need to reconfigure?
extern int GLOB_RUNNING; // Am I running?
extern int GLOB_STOPPING; // Do I need to stop?

//Global Params
extern int GLOB_NUMFRAMES; // Number of frames per FITS file
extern int GLOB_COADD; // Flag as to whether to coadd frames on save
extern int GLOB_IMSIZE; // Size of one image in pixels
extern int GLOB_WIDTH; // Width of image in pixels

extern int GLOB_WIDTH_MAX;
extern int GLOB_WIDTH_MIN;
extern int GLOB_HEIGHT_MAX;
extern int GLOB_HEIGHT_MIN;
extern int GLOB_GAIN_MAX;
extern int GLOB_GAIN_MIN;
extern int GLOB_EXPTIME_MAX;
extern int GLOB_EXPTIME_MIN;
extern double GLOB_BLACKLEVEL_MAX;
extern double GLOB_BLACKLEVEL_MIN;

//Main configuration file (TOML)
extern char * GLOB_CONFIGFILE ;

//Thread for running the camera
extern pthread_t GLOB_CAMTHREAD;

//pThread Locks
extern pthread_mutex_t GLOB_FLAG_LOCK;
extern pthread_mutex_t GLOB_LATEST_FILE_LOCK;
extern pthread_mutex_t GLOB_LATEST_IMG_INDEX_LOCK;
extern pthread_mutex_t *GLOB_IMG_MUTEX_ARRAY;

// Array of images (i.e Image buffer)
extern unsigned short *GLOB_IMG_ARRAY;

// Latest filename/image
extern std::string GLOB_LATEST_FILE ;
extern int GLOB_LATEST_IMG_INDEX;

//configuration struct of various camera parameters. Can be serialised to/from JSON
struct configuration{
    float gain; //Gain
    float exptime; //Exposure time
    int width;  //Width
    int height;  //Height
    int offsetX;  //X offset
    int offsetY;  //Y offset
    float blacklevel; //Black level
    int buffersize; //Size of the circular buffer in units of frames
    std::string savedir; //Save directory filename prefix
};

//Global configuration struct instance
extern configuration GLOB_CONFIG_PARAMS;

//Sinc function
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
