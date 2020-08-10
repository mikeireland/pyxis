// Based on the work done by https://github.com/miks/spinnaker-fps-test

#ifndef _ACQUISITION_
#define _ACQUISITION_

#include "Spinnaker.h"
#include "cpptoml/cpptoml.h"

struct times{
    int totalexposure;
    char* timestamp;
};

std::string Label(std::string str, const size_t num = 20, const char paddingChar = ' ');

void grabFrames(Spinnaker::CameraPtr pCam, std::shared_ptr<cpptoml::table> config, unsigned long numFrames, std::vector<int> fitsArray, struct times timesStruct, int useFunc, void (*f)(unsigned char*));

#endif // _ACQUISITION_
