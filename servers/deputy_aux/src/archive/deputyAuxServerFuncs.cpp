
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <time.h>
#include <pthread.h>
#include "deputyAuxGlobals.hpp"
#include "SerialPort.h"

#include <cstdlib>

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

Comms::SerialPort teensy_port(128);

void * serverLoop(void *){

    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_DA_STATUS = 1; // Connecting
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

    auto time_start = steady_clock::now();
    auto time_current = steady_clock::now();

    //START UP TEENSY COMMS

    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_DA_STATUS = 2; // Running/waiting
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);  

    time_start = steady_clock::now();
    
    while(GLOB_DA_STATUS > 0){
        switch(GLOB_DA_REQUEST) {
			case 1:
				cout << "TURNING LED ON" << endl;
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
		        GLOB_DA_REQUEST = 0;
		        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
                break;
			case 2:
				cout << "TURNING LED OFF" << endl;
		        pthread_mutex_lock(&GLOB_FLAG_LOCK);
		        GLOB_DA_REQUEST = 0;
		        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				break;
			default: //if no correct flag, sleep
				usleep(1000);
				break;
		}

		time_current = steady_clock::now();

		auto delta_time = duration_cast<milliseconds>(time_current-time_start).count();

        if(delta_time>GLOB_DA_POWER_REQUEST_TIME){

            // Request power info
            
            powerStruct result; //test data
            result.current = rand() % 100;
            result.voltage = rand() % 100;
            
            pthread_mutex_lock(&GLOB_FLAG_LOCK);
            GLOB_DA_POWER_VALUES = result;
            pthread_mutex_unlock(&GLOB_FLAG_LOCK);
            
            time_start = steady_clock::now();
        }
    }

    
    pthread_exit(NULL);

    // Shut down Teensy comms
}


// FLIR Camera Server
struct DeputyAuxServer {

    DeputyAuxServer()
    {
        fmt::print("DeputyAuxServer\n");
    }

    ~DeputyAuxServer()
    {
        fmt::print("~DeputyAuxServer\n");
    }




int turnLEDOn(){
    int ret_msg;
	teensy_port.Request(LEDON);
	teensy_port.SendAllRequests();
	usleep(150);
	teensy_port.ReadMessage();
	ret_msg = teensy_port.ledOn;
	return ret_msg;
}


int turnLEDOff(){
    int ret_msg;
	teensy_port.Request(LEDOFF);
	teensy_port.SendAllRequests();
	usleep(150);
	teensy_port.ReadMessage();
	ret_msg = !teensy_port.ledOn;
	teensy_port.ledOn = false;
	return ret_msg;
}


powerStatus requestPower(){
    powerStatus ret_p;
	powerStruct p;
    string ret_msg;
    
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    p = GLOB_DA_POWER_VALUES;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    
    ret_p.current = p.current;
    ret_p.voltage = p.voltage;
    
    ret_msg = "Voltage = " + to_string(p.voltage) + ", Current = " + to_string(p.current);
    
    ret_p.msg = ret_msg;
    
    return ret_p;
}

};

// Serialiser to convert configuration struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<powerStruct> {
        static void to_json(json& j, const powerStruct& p) {
            j = json{{"current", p.current}, {"voltage", p.voltage}};
        }

        static void from_json(const json& j, powerStruct& p) {
            j.at("current").get_to(p.current);
            j.at("voltage").get_to(p.voltage);
        }
    };
}

namespace nlohmann {
    template <>
    struct adl_serializer<powerStatus> {
        static void to_json(json& j, const powerStatus& p) {
            j = json{{"current", p.current}, {"voltage", p.voltage}, {"message", p.msg}};
        }

        static void from_json(const json& j, powerStatus& p) {
            j.at("current").get_to(p.current);
            j.at("voltage").get_to(p.voltage);
            j.at("message").get_to(p.msg);
        }
    };
}

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<DeputyAuxServer>("DA")
        // To insterface a class method, you can use the `def` method.
        .def("LEDOn", &DeputyAuxServer::turnLEDOn, "Turn on the LED")
        .def("LEDOff", &DeputyAuxServer::turnLEDOff, "Turn off the LED")
        .def("reqpower", &DeputyAuxServer::requestPower, "Request power values");

}
