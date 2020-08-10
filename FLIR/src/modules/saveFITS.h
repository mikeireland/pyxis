#ifndef _SAVEFITS_
#define _SAVEFITS_

#include "cpptoml/cpptoml.h"
#include "acquisition.h"

int saveFITS(std::shared_ptr<cpptoml::table> config, std::vector<int> fitsArray, times timesStruct);

#endif // _SAVEFITS_
