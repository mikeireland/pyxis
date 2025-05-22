#include <commander/commander.h>
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"
#include "Globals.h"

namespace co = commander;
using namespace std;

double roll_gain = -0.08;
double pitch_gain = -0.08;

double roll_target = 0;
double pitch_target = 0;


// Main server function. Accepts one parameter: link to the camera config file.
int main(int argc, char* argv[]) {

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
        //config_file = string("config/defaultQHYLocalConfig.toml");
        config_file = string("config/defaultLocalConfig.toml");
    } else {
        // Assign config file value as string
        config_file = string(argv[1]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    toml::table config = toml::parse_file(config_file);

    // Add the roll and pitch gain from the config file
    if (config["roll_gain"].is_number()) {
        roll_gain = config["roll_gain"].value_or(roll_gain);
    } else {
        cout << "Roll gain not found in config file, using default value" << endl;
    }
    if (config["pitch_gain"].is_number()) {
        pitch_gain = config["pitch_gain"].value_or(pitch_gain);
    } else {
        cout << "Pitch gain not found in config file, using default value" << endl;
    }

    // Add the roll and pitch target from the config file
    if (config["roll_target"].is_number()) {
        roll_target = config["roll_target"].value_or(roll_target);
    } else {
        cout << "Roll target not found in config file, using default value" << endl;
    }
    if (config["pitch_target"].is_number()) {
        pitch_target = config["pitch_target"].value_or(pitch_target);
    } else {
        cout << "Pitch target not found in config file, using default value" << endl;
    }


    // Retrieve port and IP
    string port = config["port"].value_or("4200");
    string IP = config["IP"].value_or("192.168.1.4");
    
    
    // Turn into a TCPString
    string TCPString = "tcp://" + IP + ":" + port;
    char TCPCharArr[TCPString.length() + 1];
    strcpy(TCPCharArr, TCPString.c_str());
    
    if (config["mode"]=="Interactive") {
    	argc = 1;
    	char* argv_new[1];

    	co::Server s(argc, argv_new);
    
    	s.run();
    } else {

        // Argc/Argv to turn into the required server input
        argc = 3;
        char* argv_new[3];
        argv_new[1] = (char*)"--socket";
        argv_new[2] = TCPCharArr;

        // Start server!
        co::Server s(argc, argv_new);

        s.run();
    }
}
