#ifndef _ZABERACTUATOR_
#define _ZABERACTUATOR_

#include <zaber/motion/binary.h>
#include "toml.hpp"

class ZaberActuator{

    public:

        toml::table config;

        zaber::motion::binary::Device * pDev;

        zaber::motion::binary::Connection * pCon;

        ZaberActuator(std::string serial_port, toml::table config_init);

};

#endif // _ZABERACTUATOR_
