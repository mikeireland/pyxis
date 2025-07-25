#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <time.h>
#include "SerialPort.h"

#include <cstdlib>
//#define PRINT_DEBUG

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

Comms::SerialPort teensy_port(127);

/*
Struct to hold voltages and currents for server requests
*/
struct powerStatus{
    double PC_V;
    double PC_A;
    double motor_V;
    double motor_A;
    string message; //return message
};

// Initialise status
powerStatus PS;

// Auxillary Server
struct DeputyAuxServer {

    DeputyAuxServer()
    {
        fmt::print("DeputyAuxServer\n");
    }

    ~DeputyAuxServer()
    {
        fmt::print("~DeputyAuxServer\n");
    }

    /*
    Command to turn the LED on
    Returns 1 if successful (on), 0 if off
    */
    int turnLEDOn(){
        int ret_msg;
	    teensy_port.Request(LEDON);
	    teensy_port.SendAllRequests();
	    usleep(150);
	    teensy_port.ReadMessage();
	    ret_msg = teensy_port.ledOn;
	    return ret_msg;
    }

    /*
    Command to turn the LED off
    Returns 1 if successful (off), 0 if on
    */
    int turnLEDOff(){
        int ret_msg;
	    teensy_port.Request(LEDOFF);
	    teensy_port.SendAllRequests();
	    usleep(150);
	    teensy_port.ReadMessage();
	    ret_msg = !teensy_port.ledOn;
	    return ret_msg;
    }

    /*
    Function to read the wattmeter, and pass the updated struct to the client.
    Returns the powerStatus struct (serialised to JSON)
    */
    powerStatus requestPower(){
        string ret_msg;
        
        readWattmeter();
        
        ret_msg = "PC Voltage = " + to_string(PS.PC_V) + ", PC Current = " + to_string(PS.PC_A) + 
                  ", Motor Voltage = " + to_string(PS.motor_V) + ", Motor Current = " + to_string(PS.motor_A);
        
        PS.message = ret_msg;
        
        return PS;
    }

    /*
    Function to read the wattmeter on the teensy, and store values in the global struct
    */
    void readWattmeter() {
        teensy_port.Request(WATTMETER);
	    teensy_port.SendAllRequests();
	    usleep(150);
	    teensy_port.ReadMessage();
	    
        // Store values
	    PS.PC_V = teensy_port.PC_Voltage;
	    PS.PC_A = teensy_port.PC_Current;
	    PS.motor_V = teensy_port.Motor_Voltage;
	    PS.motor_A = teensy_port.Motor_Current;

#ifdef PRINT_DEBUG
        cout << "Power Status: " << endl;
	    cout << "PC Voltage (mV): " << PS.PC_V << endl;
	    cout << "PC Current (mA): " << PS.PC_A << endl;
	    cout << "Motor Voltage (mV): " << PS.motor_V << endl;
	    cout << "Motor Current (mA): " << PS.motor_A << endl;
#endif
    }
    
    /*
    Get the heading from the compass.
    Returns the heading as an uint16
    */
    uint16_t getHeading() {
        teensy_port.Request(COMPASS);
	    teensy_port.SendAllRequests();
	    usleep(150);
	    teensy_port.ReadMessage();
	    return teensy_port.heading;
    }


};

// Serialiser to convert power status struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<powerStatus> {
        static void to_json(json& j, const powerStatus& ps) {
            j = json{{"PC_current", ps.PC_A}, 
                     {"PC_voltage", ps.PC_V},
                     {"Motor_current", ps.motor_A}, 
                     {"Motor_voltage", ps.motor_V},
                     {"message",ps.message}};
        }

        static void from_json(const json& j, powerStatus& ps) {
            j.at("PC_current").get_to(ps.PC_A);
            j.at("PC_voltage").get_to(ps.PC_V);
            j.at("Motor_current").get_to(ps.motor_A);
            j.at("Motor_voltage").get_to(ps.motor_V);
            j.at("message").get_to(ps.message);
        }
    };
}

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<DeputyAuxServer>("DA")
        .def("LEDOn", &DeputyAuxServer::turnLEDOn, "Turn on the LED")
        .def("LEDOff", &DeputyAuxServer::turnLEDOff, "Turn off the LED")
        .def("heading", &DeputyAuxServer::getHeading, "Get the latest compass heading")
        .def("reqpower", &DeputyAuxServer::requestPower, "Request power values")
        .def("status", &DeputyAuxServer::requestPower, "Get current PC voltage"); // Qianhui added this line so DA.status exists.

}
