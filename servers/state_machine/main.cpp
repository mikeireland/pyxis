#include <commander/commander.h>
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"

namespace co = commander;
using namespace std;

struct coord{
    double RA; //current
    double DEC; //voltage
};

coord GLOB_SM_COORD;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";


struct StateMachineServer {

    StateMachineServer()
    {
        fmt::print("DeputyAuxServer\n");
    }

    ~StateMachineServer()
    {
        fmt::print("~DeputyAuxServer\n");
    }

string setCoordinates(double new_ra, double new_dec){
    string ret_msg;
    GLOB_SM_COORD.RA = new_ra;
    GLOB_SM_COORD.DEC = new_dec;
    ret_msg = "Set coordinates to " + to_string(new_ra) + ", " + to_string(new_dec);
	return ret_msg;
}

coord getCoordinates(){
	coord ret_coord;
	ret_coord = GLOB_SM_COORD;
	return ret_coord;
}

};


// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<StateMachineServer>("SM")
        // To insterface a class method, you can use the `def` method.
        .def("getCoordinates", &StateMachineServer::getCoordinates, "Set Ra and Dec")
        .def("setCoordinates", &StateMachineServer::setCoordinates, "Get Ra and Dec");
}

// Serialiser to convert coord struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<coord> {
        static void to_json(json& j, const coord& c) {
            j = json{{"RA", c.RA}, {"DEC", c.DEC}};
        }

        static void from_json(const json& j, coord& c) {
            j.at("RA").get_to(c.RA);
            j.at("DEC").get_to(c.DEC);
        }
    };
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
    
    GLOB_SM_COORD.RA = config["coords"]["RA"].value_or(0.0);
    GLOB_SM_COORD.DEC = config["coords"]["DEC"].value_or(0.0);

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
