// These are *not* globals, but only local globals within robotControllerServerFuncs,
// and should be moved to there.

double roll_gain = -0.08;
double pitch_gain = -0.08;
double roll_int = 0.0;
double pitch_int = 0.0;

// !!! This is a heading gain, in principle based on a magnetometer (which wasn't
// every installed??? 
double h_gain = 0.0;

double g_ygain = 0.0;
double g_egain = 0.0;

double g_yint = 0.0;
double g_eint = 0.0;