#ifndef _ZABERACTUATOR_
#define _ZABERACTUATOR_

#include <zaber/motion/binary.h>


class ZaberActuator{

    public:

		// Dimensions of image
        zaber::motion::binary::Device * pDev;

		/* Constructor: Takes the camera pointer and config table
           and saves them (and config values) as object attributes
           INPUTS:
              pCam_init - Spinnaker camera pointer
              config_init - Parsed TOML table   */
        ZaberActuator(zaber::motion::binary::Device device);

};

#endif // _ZABERACTUATOR_
