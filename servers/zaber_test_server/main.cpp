#include <commander/commander.h>
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"
#include "ZaberActuator.h"
#include <zaber/motion/binary.h>
#include <commander/client/socket.h>

namespace co = commander;
using namespace std;
using namespace zaber::motion;
using json = nlohmann::json;

commander::client::Socket* SC_SOCKET;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

ZaberActuator * stage;

struct ZaberServer {

    ZaberServer()
    {
        fmt::print("ZaberServer\n");
    }

    ~ZaberServer()
    {
        fmt::print("~ZaberServer\n");
    }

string move(double distance){
    string ret_msg;

    double position = stage->MoveRelative(distance, Units::VELOCITY_METRES_PER_SECOND);
    cout << position << endl;

	return ret_msg;
}

//distance in m/frame
string move_loop(double distance_per_frame){
    string ret_msg;
    string ret_msg = SC_SOCKET->send<std::string>("setupZaber", distance_per_frame);
    cout << ret_msg << endl;
    ofstream myfile;
    myfile.open ("example.txt");
    for (int k=0; k<100; k++){
        double position = stage->MoveRelative(distance/2, Units::METRES);
        cout << position << endl;
        string FFTdata = SC_SOCKET->send<std::string>("zaberFunc");
        cout << FFTdata << endl;
        
        json j = json::parse(FFTdata)
        vector<double> vec = j.at["SNR"];
        cout << vec[0] << endl;
        myfile << FFTdata + "\n";
        
    }
    myfile.close();
	return ret_msg;
}

};


// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<ZaberServer>("ZS")
        // To insterface a class method, you can use the `def` method.
        .def("move", &ZaberServer::move, "Get Ra and Dec")
        .def("move_loop", &ZaberServer::move_loop, "Get Target Name");
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
    
    std::string SC_port = config["ScienceCamera"]["SC_port"].value_or("4100");

    // Turn into a TCPString
    std::string SC_TCP = "tcp://" + IP + ":" + SC_port;
    
    SC_SOCKET = new commander::client::Socket(SC_TCP);

    // Get the settings for the particular actuator
    toml::table stage_config = *config.get("testZaberActuator")->as_table();

    // Initialise ZaberActuator instance for the stage at the port in the config file
    ZaberActuator STAGE (stage_config);
    
    stage = &STAGE;
    
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
