#include <cmath>
#include <string>
#include <pthread.h>
#include "globals.h"
#include <functional>

using namespace std;

//Flags
int GLOB_CAM_STATUS = 0;
int GLOB_RECONFIGURE = 0;
int GLOB_RUNNING = 0;
int GLOB_STOPPING = 0;

//Global Params
int GLOB_NUMFRAMES = 0;
int GLOB_COADD = 0;
int GLOB_IMSIZE = 0;
int GLOB_WIDTH = 0;

int GLOB_WIDTH_MAX = 0;
int GLOB_WIDTH_MIN = 0;
int GLOB_HEIGHT_MAX = 0;
int GLOB_HEIGHT_MIN = 0;
int GLOB_GAIN_MAX = 0;
int GLOB_GAIN_MIN = 0;
int GLOB_EXPTIME_MAX = 0;
int GLOB_EXPTIME_MIN = 0;
double GLOB_BLACKLEVEL_MAX = 0;
double GLOB_BLACKLEVEL_MIN = 0;


//config_file
char* GLOB_CONFIGFILE = (char*)"./";

//Thread!!!!! POSSIBLY WRONG!!!!!! (Seems to work though?)
pthread_t GLOB_CAMTHREAD = 0;

//Locks
pthread_mutex_t GLOB_FLAG_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GLOB_LATEST_FILE_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GLOB_LATEST_IMG_INDEX_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *GLOB_IMG_MUTEX_ARRAY;

// Array of images (i.e Image buffer)
unsigned short *GLOB_IMG_ARRAY;

std::function<int(unsigned short*)> GLOB_CALLBACK;

// Latest file/image
string GLOB_LATEST_FILE = "NOFILESAVED";
int GLOB_LATEST_IMG_INDEX = 0;

//Global configuration struct instance
configuration GLOB_CONFIG_PARAMS;

extern const double kPi = 3.141592654; //Pi

//Sinc function
double sinc(double x){
    if (x == 0){
        return 1.0;
    } else{
    double result = sin(kPi*x)/(kPi*x);
    return result;
}
}

/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num, const char padding_char) {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), padding_char);
        }
        return str + ": ";
    }
