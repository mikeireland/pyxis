#include <iostream>
#include <vector>
#include "../../lib/cpptoml/cpptoml.h"
#include "fitsio.h"  /* required by every program that uses CFITSIO  */


int saveFITS(string filename, auto config, vector<int> fitsArray, times timesStruct);
