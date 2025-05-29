#include <commander/commander.h>
#include <string>
#include <fstream>
#include <iostream>
#include "toml.hpp"

namespace co = commander;
using namespace std;

/*
Struct to hold Ra and Dec coordinates
*/
struct coord{
    double RA; //current
    double DEC; //voltage
};

// Global target parameters for requests
coord GLOB_TS_COORD;
string GLOB_TS_NAME;
double GLOB_TS_BASELINE;

//config_file
char* GLOB_CONFIGFILE = (char*)"./";

// Target server definition
struct TargetServer {

    TargetServer()
    {
        fmt::print("DeputyAuxServer\n");
    }

    ~TargetServer()
    {
        fmt::print("~DeputyAuxServer\n");
    }

    /*
    Function to set a new target's coordinates
    Inputs:
        new_ra - new right ascension (deg)
        new_dec - new declination (deg)
    Output:
        status string
    */
    string setCoordinates(double new_ra, double new_dec){
        string ret_msg;
        GLOB_TS_COORD.RA = new_ra;
        GLOB_TS_COORD.DEC = new_dec;
        ret_msg = "Set coordinates to " + to_string(new_ra) + ", " + to_string(new_dec);
        return ret_msg;
    }

    /*
    Function to set a new target's name
    Inputs:
        target_name - name of target
    Output:
        status string
    */
    string setTargetName(string target_name){
        string ret_msg;
        GLOB_TS_NAME = target_name;
        ret_msg = "Set Target Name to " + target_name;
        return ret_msg;
    }

    /*
    Function to set a new baseline length
    Inputs:
        baseline
    Output:
        status string
    */
    string setBaseline(double baseline){
        string ret_msg;
        GLOB_TS_BASELINE = baseline;
        ret_msg = "Set baseline " + to_string(baseline);
        return ret_msg;
    }

    /* 
    Function to get the current target coordinates, in the form of a coord struct
    */
    coord getCoordinates(){
        coord ret_coord;
        ret_coord = GLOB_TS_COORD;
        return ret_coord;
    }

    /* 
    Function to get the current target's name as a string
    */
    string getTargetName(){
        string name = GLOB_TS_NAME;
        return name;
    }

    /*
    Function to get the current baseline
    */
    double getBaseline(){
        double baseline = GLOB_TS_BASELINE;
        return baseline;
    }

    /*
    Function to check if the server is alive!
    */
    string status(){
        string ret_msg;
        ret_msg = "Server Running";
        return ret_msg;
    }

};


// Register with commander
COMMANDER_REGISTER(m)
{
    m.instance<TargetServer>("TS")
        .def("getCoordinates", &TargetServer::getCoordinates, "Get Ra and Dec")
        .def("getTargetName", &TargetServer::getTargetName, "Get Target Name")
        .def("getBaseline", &TargetServer::getBaseline, "Get Baseline")
        .def("setCoordinates", &TargetServer::setCoordinates, "Set Ra and Dec in degrees [Ra, Dec]")
        .def("setTargetName", &TargetServer::setTargetName, "Set Target Name [Name]")
        .def("setBaseline", &TargetServer::setBaseline, "Set Baseline [Baseline]")
        .def("status", &TargetServer::status, "Check status");
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
    
    // Set coords as (0,0)
    GLOB_TS_COORD.RA = 0.0;
    GLOB_TS_COORD.DEC = 0.0;
    
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
