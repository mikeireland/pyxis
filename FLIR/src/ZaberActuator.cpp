#include <zaber/motion/binary.h>
#includ "toml.hpp"
#include "ZaberActuator.h"

using namespace zaber::motion;
using namespace zaber::motion::binary;


ZaberActuator::ZaberActuator(std::string serial_port, toml::table config_init){

    config = config_init;

    std::string serial_port = config["zaber"]["serial_port"].value_or("");

    Library::enableDeviceDbStore();

    pCon = new Connection;
    pCon->openSerialPort(serial_port);

    std::vector<Device> deviceList = pCon->detectDevices();
    std::cout << "Found " << deviceList.size() << " devices." << std::endl;

    pDev = new Device(deviceList[0]);

    pDev->home();

}
