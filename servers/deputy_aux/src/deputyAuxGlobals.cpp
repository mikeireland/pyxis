#include <pthread.h>
#include "deputyAuxGlobals.hpp"

int GLOB_DA_STATUS = 0; //0 = Off, 1 = On, 2 = waiting
int GLOB_DA_REQUEST = 0; //0 = no command, 1 = turn LED on, 2 = turn LED off

//Global power struct instance
powerStruct GLOB_DA_POWER_VALUES;

int GLOB_DA_POWER_REQUEST_TIME;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

//Thread!!!!! POSSIBLY WRONG!!!!!! (Seems to work though?)
pthread_t GLOB_DA_THREAD = 0;

//Locks
pthread_mutex_t GLOB_FLAG_LOCK = PTHREAD_MUTEX_INITIALIZER;

