#include <pthread.h>
#include "chiefAccGlobals.hpp"

int GLOB_CA_STATUS; //0 = Off, 1 = On, 2 = waiting
int GLOB_CA_REQUEST; //1 = no command, 2 = turn LED on, 3 = turn LED off, 4 = request power

//Global power struct instance
powerStruct GLOB_CA_POWER_VALUES;

extern int GLOB_CA_FINESTAGE_STEPS;
extern double GLOB_CA_SCIPIEZO_VOLTAGE;
extern double GLOB_CA_TIPTILT_PIEZO_DEXTRA;
extern double GLOB_CA_TIPTILT_PIEZO_SINISTRA;

int GLOB_CA_POWER_REQUEST_TIME;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

//Thread!!!!! POSSIBLY WRONG!!!!!! (Seems to work though?)
pthread_t GLOB_CA_THREAD = 0;

//Locks
pthread_mutex_t GLOB_FLAG_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GLOB_DATA_LOCK = PTHREAD_MUTEX_INITIALIZER;

