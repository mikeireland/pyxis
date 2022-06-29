
#include <commander/commander.h>
#include "globals.h"
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"

namespace co = commander;
using namespace std;


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
        config_file = string("config/defaultConfig.toml");
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
    
    GLOB_CONFIGFILE = (char*)config_file.c_str();
    
    string port = config["port"].value_or("4000");
    string IP = config["IP"].value_or("192.168.1.4");
    
    string TCPString = "tcp://" + IP + ":" + port;
    char TCPCharArr[TCPString.length() + 1];
    strcpy(TCPCharArr, TCPString.c_str());
    
    argc = 3;
    char* argv_new[3];
    argv_new[1] = (char*)"--socket";
    argv_new[2] = TCPCharArr;

    co::Server s(argc, argv_new);
    
    s.run();
}
