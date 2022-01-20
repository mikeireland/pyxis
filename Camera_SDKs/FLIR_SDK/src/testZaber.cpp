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

    // Get the settings for the particular actuator
    toml::table stage_config = *config.get("testZaberActuator")->as_table();

    cout << endl;
    cout << "Initializing" << endl;
    cout << endl;
    // Initialise ZaberActuator instance for the stage at the port in the config file
    ZaberActuator stage (stage_config);

    //Perform a test by moving the actuator 1 mm from home
    cout << endl;
    cout << "Move absolute Test" << endl;
    cout << endl;

    stage.MoveAbsolute(1, Units::LENGTH_MILLIMETRES);

    cout << "Press the Enter key to continue" << endl;
    getchar();

    // Perform a test to move the actuator at a constant speed of 1um/s
    // until the enter button is pressed.
    cout << endl;
    cout << "Move Velocity Test" << endl;
    cout << endl;

    stage.MoveAtVelocity(1, Units::VELOCITY_MICROMETRES_PER_SECOND);
    cout << "Press the Enter key to stop" << endl;
    getchar();
    stage.Stop(Units::LENGTH_MILLIMETRES);

    return 0;
}
