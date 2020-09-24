#ifndef _ZABERACTUATOR_
#define _ZABERACTUATOR_

#include <zaber/motion/binary.h>
#include "toml.hpp"

class ZaberActuator{

    public:

        toml::table config;

        str::string serial_port;
        int device_number;

        ZaberActuator(toml::table config_init);

        Message IssueCommand(const zaber::motion::binary::CommandCode command, const double data, const double timeout = 0.5, const zaber::motion::Units unit = zaber::motion::Units::NATIVE)

        void MoveAbsolute(double position, const zaber::motion::Units unit = zaber::motion::Units::NATIVE, const double timeout = 60);

        void MoveRelative(double position, const zaber::motion::Units unit = zaber::motion::Units::NATIVE, const double timeout = 60);

        void MoveAtVelocity(double velocity, const zaber::motion::Units unit = zaber::motion::Units::NATIVE, const double timeout = 0.5);

        void Stop(const zaber::motion::Units unit = zaber::motion::Units::NATIVE, const double timeout = 60);

        void ConfigureSetting(const zaber::motion::binary::BinarySettings setting, const double value, const zaber::motion::Units unit = zaber::motion::Units::NATIVE);

    private:

        void SetDeviceMode();

};

#endif // _ZABERACTUATOR_
