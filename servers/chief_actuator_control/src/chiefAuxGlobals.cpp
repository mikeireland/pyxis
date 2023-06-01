#include <pthread.h>
#include "chiefAuxGlobals.hpp"

int GLOB_CA_STATUS = 0; //0 = Off, 1 = On, 2 = waiting
int GLOB_CA_REQUEST = 0; //1 = no command, 2 = turn LED on, 3 = turn LED off, 4 = request power

//Global power struct instance
powerStruct GLOB_CA_POWER_VALUES;

int GLOB_CA_FINESTAGE_STEPS = 0;
int GLOB_CA_FINESTAGE_DIRECTION = 0;
int GLOB_CA_FINESTAGE_FREQUENCY = 0;
double GLOB_CA_SCIPIEZO_VOLTAGE = 0;
double GLOB_CA_TIPTILT_PIEZO_DEXTRA_X = 0;
double GLOB_CA_TIPTILT_PIEZO_DEXTRA_Y = 0;
double GLOB_CA_TIPTILT_PIEZO_SINISTRA_X = 0;
double GLOB_CA_TIPTILT_PIEZO_SINISTRA_Y = 0;
double GLOB_CA_TIPTILT_VOLTAGE_FACTOR = 0;
double GLOB_CA_TIPTILT_PIXEL_CONVERSION = 0;
double CURRENT_DEXTRA_X_VOLTAGE = 0;
double CURRENT_DEXTRA_Y_VOLTAGE = 0;
double CURRENT_SINISTRA_X_VOLTAGE = 0;
double CURRENT_SINISTRA_Y_VOLTAGE = 0;

int GLOB_CA_POWER_REQUEST_TIME;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

//Thread!!!!! POSSIBLY WRONG!!!!!! (Seems to work though?)
pthread_t GLOB_CA_THREAD = 0;

//Locks
pthread_mutex_t GLOB_FLAG_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GLOB_DATA_LOCK = PTHREAD_MUTEX_INITIALIZER;

