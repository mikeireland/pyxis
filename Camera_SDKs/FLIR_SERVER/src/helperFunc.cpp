#include <cmath>
#include <string>
#include <pthread.h>
#include "helperFunc.h"

using namespace std;

//Flags
int GLOB_CAM_STATUS = 0;
int GLOB_RECONFIGURE = 0;
int GLOB_RUNNING = 0;
int GLOB_STOPPING = 0;
int GLOB_NUMFRAMES = 0;

//config_file
char* GLOB_CONFIGFILE = (char*)"config/defaultConfig.toml";

pthread_mutex_t flag_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t img_array_lock = PTHREAD_MUTEX_INITIALIZER;

//Thread!!!!! POSSIBLY VERY WRONG!!!!!!
pthread_t camthread = 0;


extern const double kPi = 3.141592654;

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
