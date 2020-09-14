#include <zaber/motion/binary.h>
#include "ZaberActuator.h"

using namespace zaber::motion;
using namespace zaber::motion::binary;


ZaberActuator::ZaberActuator(zaber::motion::binary::Device device){

    pDev = &device;
    pDev->home();

}

