#include <commander/commander.h>
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"
#include "ZaberActuator.h"
#include <zaber/motion/binary.h>

namespace co = commander;
using namespace std;
using namespace zaber::motion;

coord GLOB_TS_COORD;
string GLOB_TS_NAME;
double GLOB_TS_BASELINE;

commander::client::Socket* SC_SOCKET;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

ZaberActuator STAGE;

struct ZaberServer {

    TargetServer()
    {
        fmt::print("DeputyAuxServer\n");
    }

    ~TargetServer()
    {
        fmt::print("~DeputyAuxServer\n");
    }

string move(double distance){
    string ret_msg;

    double position = stage.MoveRelative(distance, Units::VELOCITY_METRES_PER_SECOND);
    cout << position << endl;

	return ret_msg;
}

string move_loop(double distance){
    string ret_msg;
    
    for (int k=0; k<100; k++){
        double position = stage.MoveRelative(distance, Units::VELOCITY_METRES_PER_SECOND);
        cout << position << endl;
        string FFTdata = SC_SOCKET->send<std::string>("getSNRarray");
        cout << FFTdata << endl;
    }
	return ret_msg;
}

};


// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<TargetServer>("TS")
        // To insterface a class method, you can use the `def` method.
        .def("getCoordinates", &TargetServer::getCoordinates, "Get Ra and Dec")
        .def("getTargetName", &TargetServer::getTargetName, "Get Target Name")
        .def("getBaseline", &TargetServer::getBaseline, "Get Baseline")
        .def("setCoordinates", &TargetServer::setCoordinates, "Set Ra and Dec")
        .def("setTargetName", &TargetServer::setTargetName, "Set Target Name")
        .def("setBaseline", &TargetServer::setBaseline, "Set Baseline")
        .def("status", &TargetServer::status, "Check status");
}


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

    // Retrieve port and IP
    string port = config["port"].value_or("4000");
    string IP = config["IP"].value_or("192.168.1.4");
    
    std::string SC_port = config["ScienceCamera"]["CA_port"].value_or("4100");

    // Turn into a TCPString
    std::string SC_TCP = "tcp://" + IP + ":" + CA_port;
    
    SC_SOCKET = new commander::client::Socket(CA_TCP);
    GLOB_TS_COORD.RA = 0.0;
    GLOB_TS_COORD.DEC = 0.0;
    
    // Get the settings for the particular actuator
    toml::table stage_config = *config.get("testZaberActuator")->as_table();

    // Initialise ZaberActuator instance for the stage at the port in the config file
    stage (stage_config);
    
    // Turn into a TCPString
    string TCPString = "tcp://" + IP + ":" + port;
    char TCPCharArr[TCPString.length() + 1];
    strcpy(TCPCharArr, TCPString.c_str());

    // Argc/Argv to turn into the required server input
    argc = 3;
    char* argv_new[3];
    argv_new[1] = (char*)"--socket";
    argv_new[2] = TCPCharArr;

    // Start server!
    co::Server s(argc, argv_new);

    s.run();
}
