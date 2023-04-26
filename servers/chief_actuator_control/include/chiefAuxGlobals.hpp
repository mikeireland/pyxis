#ifndef _CHIEFAUXGLOBALS_
#define _CHIEFAUXGLOBALS_

#include <pthread.h>

//Flags for server to communicate status.
extern int GLOB_CA_STATUS; //0 = Off, 1 = On, 2 = waiting
extern int GLOB_CA_REQUEST; //1 = no command, 2 = turn LED on, 3 = turn LED off, 4 = request power

//Power parameters struct. Can be serialised to/from JSON
struct powerStruct{
    float current; //current
    float voltage; //voltage
};

//Global power struct instance
extern powerStruct GLOB_CA_POWER_VALUES;

extern int GLOB_CA_FINESTAGE_STEPS;
extern double GLOB_CA_SCIPIEZO_VOLTAGE;
extern double GLOB_CA_TIPTILT_PIEZO_DEXTRA;
extern double GLOB_CA_TIPTILT_PIEZO_SINISTRA;

extern int GLOB_CA_POWER_REQUEST_TIME;

//Main configuration file (TOML)
extern char * GLOB_CONFIGFILE ;

//Thread for running the camera
extern pthread_t GLOB_CA_THREAD;

//pThread Locks
extern pthread_mutex_t GLOB_FLAG_LOCK;
extern pthread_mutex_t GLOB_DATA_LOCK;

#endif // _CHIEFAUXGLOBALS_
