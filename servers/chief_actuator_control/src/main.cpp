#include <commander/commander.h>
#include "chiefAuxGlobals.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"

namespace co = commander;
using namespace std;


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

    // Set config file path as a global variable
    GLOB_CONFIGFILE = (char*)config_file.c_str();

    GLOB_CA_POWER_REQUEST_TIME = config["aux"]["power_req_time"].value_or(1000);

    // Retrieve port and IP
    string port = config["port"].value_or("4000");
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
