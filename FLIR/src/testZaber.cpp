#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "toml.hpp"
#include <zaber/motion/binary.h>
#include "ZaberActuator.h"

using namespace std;
using namespace zaber::motion;
using namespace zaber::motion::binary;

int main(int argc, char **argv) {

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    string config_file;

    // Check if config file path is passed as argument
    if (argc > 2) {
        cout << "Too many arguments!" << endl;
        exit(1);
    } else if (argc < 2){
        // Load default config if nothing is passed
        cout << "No CONFIG file loaded" << endl;
        cout << "Will attempt to load default CONFIG" << endl;
        config_file = string("../config/defaultConfig.toml");
    } else {
        // Assign config file value as string
        config_file = string("../config/") + string(argv[1]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    toml::table config = toml::parse_file(config_file);

    std::string serial_port = config["zaber"]["serial_port"].value_or("");

    Library::enableDeviceDbStore();

    Connection connection = Connection::openSerialPort(serial_port);

    std::vector<Device> deviceList = connection.detectDevices();
    std::cout << "Found " << deviceList.size() << " devices." << std::endl;

    Device device = deviceList[0];

    ZaberActuator stage (device);

    stage.pDev->moveAbsolute(10, Units::LENGTH_MILLIMETRES);




    return 0;
}
