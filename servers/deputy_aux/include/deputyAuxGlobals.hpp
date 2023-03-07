#ifndef _DEPAUXGLOBALS_
#define _DEPAUXGLOBALS_

#include <pthread.h>

//Flags for server to communicate status.
extern int GLOB_DA_STATUS; //0 = Off, 1 = On, 2 = waiting
extern int GLOB_DA_REQUEST; //1 = no command, 2 = turn LED on, 3 = turn LED off, 4 = request power

//Power parameters struct. Can be serialised to/from JSON
struct powerStruct{
    float current; //current
    float voltage; //voltage
};

//Global power struct instance
extern configuration GLOB_DA_POWER_VALUES;

extern int GLOB_DA_POWER_REQUEST_TIME;

//Main configuration file (TOML)
extern char * GLOB_CONFIGFILE ;

//Thread for running the camera
extern pthread_t GLOB_DA_THREAD;

//pThread Locks
extern pthread_mutex_t GLOB_FLAG_LOCK;

#endif // _DEPAUXGLOBALS_
