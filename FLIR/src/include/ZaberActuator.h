#ifndef _ZABERACTUATOR_
#define _ZABERACTUATOR_

#include <zaber/motion/binary.h>
#include "toml.hpp"

/* ZABER ACTUATOR CLASS
   Contains necessary methods and attributes for running
   a Zaber Actuator (Binary protocol)
*/
class ZaberActuator{

    public:

        // Configuration file for the actuator
        toml::table config;

        // Serial port the actuator is connected to
        std::string serial_port;
        // Number of the device connected to the port (generally 1)
        int device_number;

        /* Constructor: Takes the config table, connects to the actuator,
           adjusts settings, homes the device and then disconnects.
           INPUTS:
              config_init - Parsed TOML table   */
        ZaberActuator(toml::table config_init);

        /* Low(er) level function to issue a command to the device by first forming
           a connection to it, performing the command and then disconnecting.
           INPUTS:
              command - Binary CommandCode of the instruction
              data - data required for the command
              timeout - how long before timeout? (Default 0.5s)
              input_unit - what unit to use for the input to the command (default NATIVE)
              output_unit - what unit to use for reported command output (default NATIVE)
            OUTPUT:
              The output float from the command (eg position of device, velocity...)
        */
        double IssueCommand(const zaber::motion::binary::CommandCode command,
                            const double data, const double timeout = 0.5,
                            const zaber::motion::Units input_unit = zaber::motion::Units::NATIVE,
                            const zaber::motion::Units output_unit = zaber::motion::Units::NATIVE);

        /* High level function to move an absolute distance from the home position
           INPUTS:
              position - where to move to
              unit - what unit is the position in?
              timeout - how long to wait before timing out (default 60s)

           No output, but it will print the returned position to the screen
        */
        void MoveAbsolute(double position, const zaber::motion::Units unit = zaber::motion::Units::NATIVE,
                          const double timeout = 60);

        /* High level function to move a relative distance from the current position
           INPUTS:
              position - where to move to
              unit - what unit is the position in?
              timeout - how long to wait before timing out (default 60s)

           No output, but it will print the returned position to the screen
        */
        void MoveRelative(double position, const zaber::motion::Units unit = zaber::motion::Units::NATIVE,
                          const double timeout = 60);

        /* High level function to start moving at a certain speed
           INPUTS:
              velocity - how fast to move
              unit - what unit is the position in?
              timeout - how long to wait before timing out (default 0.5s). As
                        this only initiates movement, we only require a message
                        saying the device has started moving and so the timeout
                        can be short

           No output, but it will print current speed that the device is moving at
        */
        void MoveAtVelocity(double velocity, const zaber::motion::Units unit = zaber::motion::Units::NATIVE,
                            const double timeout = 0.5);

        /* High level function to tell the device to stop moving
           INPUTS:
              unit - what unit to report the stopped position in?
              timeout - how long to wait before timing out (default 60s).

           No output, but it will print current stopped position in native units.
        */
        void Stop(const zaber::motion::Units unit = zaber::motion::Units::NATIVE,
                  const double timeout = 60);

        /* High level function to tell the device to change a setting
           INPUTS:
              setting - Binary setting to change
              value - Value for the setting
              unit - unit of the value

           No output, but it will print the device's new setting value
        */
        void ConfigureSetting(const zaber::motion::binary::BinarySettings setting,
                              const double value,
                              const zaber::motion::Units unit = zaber::motion::Units::NATIVE);

    private:

        /* From the registered configuration file, calculate the integer required
           for the device mode setting based on the list of modes set in the config file
           OUTPUTS:
              Integer to send to the device mode setting
        */
        int ReturnDeviceMode();

};

#endif // _ZABERACTUATOR_
