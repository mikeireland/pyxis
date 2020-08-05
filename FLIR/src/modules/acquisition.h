// Based on the work done by https://github.com/miks/spinnaker-fps-test

#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm>
#include "Spinnaker.h"
#include "../../lib/cpptoml/cpptoml.h"

string Label(string str, const size_t num = 20, const char paddingChar = ' ')

void grabFrames(CameraPtr pCam, auto config, int numFrames, vector<int> fitsArray, times timesStruct, int useFunc, std::function<void()> func)
