#include <zaber/motion/binary.h>
#include "toml.hpp"
#include "ZaberActuator.h"
//#include "helperFunc.h"

using namespace zaber::motion;
using namespace zaber::motion::binary;
using namespace std;

std::string Label(std::string str, const size_t num = 25, const char padding_char = ' ') {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), padding_char);
        }
        return str + ": ";
    }\
    
/* Constructor: Takes the config table, connects to the actuator,
   adjusts settings, homes the device and then disconnects.
   INPUTS:
      config_init - Parsed TOML table   */
ZaberActuator::ZaberActuator(toml::table config_init){

    // Config table
    config = config_init;

    // Read from the config table
    serial_port = config["serial_port"].value_or("");
    device_number = config["device_number"].value_or(0);

    // Initial settings
    double home_speed = config["home_speed"].value_or(0.);
    double microstep_resolution = config["microstep_resolution"].value_or(0.);
    double target_speed = config["target_speed"].value_or(0.);

    Library::enableDeviceDbStore();

    // Initiate connection with the serial port
    cout << "Opening Port: " << serial_port << endl;
    Connection connection = Connection::openSerialPort(serial_port);

    // Find the device
    std::vector<Device> deviceList = connection.detectDevices();
    cout << "Found " << deviceList.size() << " devices." << endl;
    Device device = connection.getDevice(device_number);
    cout << "Using Device Number " << device_number << endl;

    // Get the settings to initialise device
    DeviceSettings settings = device.getSettings();

    // Set various settings, including device mode
    int device_mode = ReturnDeviceMode();
    settings.set(BinarySettings::DEVICE_MODE,device_mode);
    settings.set(BinarySettings::MICROSTEP_RESOLUTION,microstep_resolution);
    //settings.set(BinarySettings::HOME_SPEED,home_speed,Units::VELOCITY_MICROMETRES_PER_SECOND);
    settings.set(BinarySettings::TARGET_SPEED,target_speed,Units::VELOCITY_MICROMETRES_PER_SECOND);

    // Print current actuator and settings information
    cout << endl << "Zaber Actuator information" << endl
         << "=========================" << endl;
    cout << Label("Device Name") << device.getName() << endl;
    cout << Label("Serial Port") << serial_port << endl;
    cout << Label("Firmware version") << device.getFirmwareVersion().getMajor() << "." << device.getFirmwareVersion().getMinor() << endl;
    cout << Label("Serial number") << device.getSerialNumber() << endl;
    cout << Label("Device Mode") << settings.get(BinarySettings::DEVICE_MODE) << endl;
    cout << Label("Home Speed (um/s)") << settings.get(BinarySettings::HOME_SPEED,Units::VELOCITY_MICROMETRES_PER_SECOND) << endl;
    cout << Label("Target Speed (um/s)") << settings.get(BinarySettings::TARGET_SPEED,Units::VELOCITY_MICROMETRES_PER_SECOND) << endl;
    cout << Label("Acceleration (um/s/s)") << settings.get(BinarySettings::ACCELERATION,Units::ACCELERATION_MICROMETRES_PER_SECOND_SQUARED) << endl;
    cout << Label("Microstep resolution") << settings.get(BinarySettings::MICROSTEP_RESOLUTION) << endl;
    cout << endl;

    // Home the device
    cout << "Homing Device" << endl;
    device.home();

    // Close the connection by forcing the connection object out of scope
    cout << "Closing Port: " << serial_port << endl;
}


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
double ZaberActuator::IssueCommand(const CommandCode command, const double data, const double timeout, const Units input_unit, const Units output_unit){
    // Open the connection and find the device
    cout << "Opening Port: " << serial_port << endl;
    Connection connection = Connection::openSerialPort(serial_port);
    std::vector<Device> deviceList = connection.detectDevices();
    Device device = connection.getDevice(device_number);

    // Give the command
    double result = device.genericCommandWithUnits(command, data, input_unit, output_unit, timeout);

    // Close the connection by forcing the connection object out of scope
    cout << "Closing Port: " << serial_port << endl;

    return result;
}


/* High level function to move an absolute distance from the home position
   INPUTS:
      position - where to move to
      unit - what unit is the position in?
      timeout - how long to wait before timing out (default 60s)
   OUTPUT:
      Will print and return the returned position
*/
double ZaberActuator::MoveAbsolute(double position, const Units unit, const double timeout){

    // Send the command to move an absolute distance
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_ABSOLUTE, position, timeout, unit, unit);

    // Print the new position
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moved to position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moved to position: " << result << " "  << getUnitLongName(unit) << endl;
    }
    return result;
}


/* High level function to move a relative distance from the current position
   INPUTS:
      position - where to move to
      unit - what unit is the position in?
      timeout - how long to wait before timing out (default 60s)
   OUTPUT:
      Will print and return the returned position
*/
double ZaberActuator::MoveRelative(double position, const Units unit, const double timeout){

    // Send the command to move a relative distance
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_RELATIVE, position, timeout, unit, unit);

    // Print the new position
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moved to position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moved to position: " << result << " "  << getUnitLongName(unit) << endl;
    }
    return result;
}


/* High level function to start moving at a certain speed
   INPUTS:
      velocity - how fast to move
      unit - what unit is the position in?
      timeout - how long to wait before timing out (default 0.5s). As
                this only initiates movement, we only require a message
                saying the device has started moving and so the timeout
                can be short
    OUTPUT:
       Will print and return current speed that the device is moving at
*/
double ZaberActuator::MoveAtVelocity(double velocity, const Units unit, const double timeout){

    // Send the command to start moving at the given velocity
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_AT_CONSTANT_SPEED, velocity, timeout, unit, unit);

    // Print the current velocity
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moving at velocity: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moving at velocity: " << result << " "  << getUnitLongName(unit) << endl;
    }
    return result;
}


/* High level function to tell the device to stop moving
   INPUTS:
      unit - what unit to report the stopped position in?
      timeout - how long to wait before timing out (default 60s).
   OUTPUT:
      Will print and return current stopped position
*/
double ZaberActuator::Stop(const Units unit, const double timeout){

    // Send the command to stop
    double result = ZaberActuator::IssueCommand(CommandCode::STOP, 0, timeout, Units::NATIVE, unit);

    // Print the current (stopped) position
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Stopped at position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Stopped at position: " << result << " "  << getUnitLongName(unit) << endl;
    }
    return result;
}


/* High level function to tell the device to change a setting
   INPUTS:
      setting - Binary setting to change
      value - Value for the setting
      unit - unit of the value

   No output, but it will print the device's new setting value
*/
void ZaberActuator::ConfigureSetting(const BinarySettings setting, const double value, const Units unit){

    // Form a connection and find the device
    cout << "Opening Port: " << serial_port << endl;
    Connection connection = Connection::openSerialPort(serial_port);
    std::vector<Device> deviceList = connection.detectDevices();
    Device device = connection.getDevice(device_number);

    // Get the settings
    DeviceSettings settings = device.getSettings();

    // Set the desired setting
    settings.set(setting,value,unit);

    // Return the value of the setting
    cout << "Value Set: " << settings.get(setting,unit) << " for "<< BinarySettings_toString(setting) << " with unit " << getUnitLongName(unit) << endl;

    // Close the connection by forcing the connection object out of scope
    cout << "Closing Port: " << serial_port << endl;
}


/* PRIVATE FUNCTION:
   From the registered configuration file, calculate the integer required
   for the device mode setting based on the list of modes set in the config file
   OUTPUTS:
      Integer to send to the device mode setting
*/
int ZaberActuator::ReturnDeviceMode(){

      int value = 0;

      // Get the array of mode bits to set
      toml::array mode_arr = *config.get("device_mode")->as_array();

      // Calculate the resultant integer from the binary bits through bit shifting
      for (std::size_t i = 0; i != mode_arr.size(); i++){
          int bit = config["device_mode"][i].value_or(0);
          value += 1 << bit;
      }

      return value;

}
