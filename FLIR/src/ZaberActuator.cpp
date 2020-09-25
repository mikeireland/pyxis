#include <zaber/motion/binary.h>
#include "toml.hpp"
#include "ZaberActuator.h"
#include "helperFunc.h"

using namespace zaber::motion;
using namespace zaber::motion::binary;
using namespace std;

ZaberActuator::ZaberActuator(toml::table config_init){

    config = config_init;

    serial_port = config["serial_port"].value_or("");
    device_number = config["device_number"].value_or(0);

    double home_speed = config["home_speed"].value_or(0.);
    double microstep_resolution = config["microstep_resolution"].value_or(0.);
    double target_speed = config["target_speed"].value_or(0.);

    Library::enableDeviceDbStore();

    cout << "Opening Port: " << serial_port << endl;

    Connection connection = Connection::openSerialPort(serial_port);

    std::vector<Device> deviceList = connection.detectDevices();
    cout << "Found " << deviceList.size() << " devices." << endl;

    Device device = connection.getDevice(device_number);
    cout << "Using Device Number " << device_number << endl;
    DeviceSettings settings = device.getSettings();

    int device_mode = ReturnDeviceMode();
    settings.set(BinarySettings::DEVICE_MODE,device_mode);
    settings.set(BinarySettings::MICROSTEP_RESOLUTION,microstep_resolution);
    settings.set(BinarySettings::HOME_SPEED,home_speed,Units::VELOCITY_MICROMETRES_PER_SECOND);
    settings.set(BinarySettings::TARGET_SPEED,target_speed,Units::VELOCITY_MICROMETRES_PER_SECOND);

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

    cout << "Homing Device" << endl;
    device.home();

    cout << "Closing Port: " << serial_port << endl;
}

double ZaberActuator::IssueCommand(const CommandCode command, const double data, const double timeout, const Units unit){
    cout << "Opening Port: " << serial_port << endl;
    Connection connection = Connection::openSerialPort(serial_port);
    std::vector<Device> deviceList = connection.detectDevices();
    Device device = connection.getDevice(device_number);
    double result = device.genericCommandWithUnits(command, data, unit, unit, timeout);
    cout << "Closing Port: " << serial_port << endl;

    return result;
}

void ZaberActuator::MoveAbsolute(double position, const Units unit, const double timeout){
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_ABSOLUTE, position, timeout, unit);
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moved to position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moved to position: " << result << " "  << getUnitLongName(unit) << endl;
    }
}

void ZaberActuator::MoveRelative(double position, const Units unit, const double timeout){
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_RELATIVE, position, timeout, unit);
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moved to position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moved to position: " << result << " "  << getUnitLongName(unit) << endl;
    }
}
void ZaberActuator::MoveAtVelocity(double velocity, const Units unit, const double timeout){
    double result = ZaberActuator::IssueCommand(CommandCode::MOVE_AT_CONSTANT_SPEED, velocity, timeout, unit);
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Moving at velocity: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Moving at velocity: " << result << " "  << getUnitLongName(unit) << endl;
    }
}

void ZaberActuator::Stop(const Units unit, const double timeout){
    double result = ZaberActuator::IssueCommand(CommandCode::STOP, 0, timeout, unit);
    if (unit != Units::NATIVE){
      std::string unitname = getUnitLongName(unit);
      cout << "Stopped at position: " << result << " "  << unitname.substr(unitname.find(":")+1) << endl;
    } else{
      cout << "Stopped at position: " << result << " "  << getUnitLongName(unit) << endl;
    }
}

void ZaberActuator::ConfigureSetting(const BinarySettings setting, const double value, const Units unit){
    cout << "Opening Port: " << serial_port << endl;
    Connection connection = Connection::openSerialPort(serial_port);
    std::vector<Device> deviceList = connection.detectDevices();
    Device device = connection.getDevice(device_number);

    DeviceSettings settings = device.getSettings();
    settings.set(setting,value,unit);
    cout << "Value Set: " << settings.get(setting,unit) << " for "<< BinarySettings_toString(setting) << " with unit " << getUnitLongName(unit) << endl;
    cout << "Closing Port: " << serial_port << endl;
}

int ZaberActuator::ReturnDeviceMode(){

      int value = 0;

      toml::array mode_arr = *config.get("device_mode")->as_array();

      for (std::size_t i = 0; i != mode_arr.size(); i++){
          int bit = config["device_mode"][i].value_or(0);
          value += 1 << bit;
      }

      return value;

}
