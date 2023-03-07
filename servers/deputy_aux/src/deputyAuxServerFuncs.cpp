
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <time.h>
#include <pthread.h>
#include <functional.h>
#include "chiefAccGlobals"

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

int serverLoop(){

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
    while(GLOB_DA_STATUS>0){

        switch(GLOB_DA_REQUEST) {
            case 1:
				usleep(10);
				break;
			case 2:
				// LED ON
                break;
			case 3:
				// LED OFF
				break;
			default: //if no correct flag, turn off
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	        GLOB_DA_STATUS = 0;
		        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				break;
		}

		time_current = steady_clock::now();

		auto delta_time = duration_cast<milliseconds>(time_current-time_start).count();

        if(delta_time>GLOB_DA_POWER_REQUEST_TIME){

            // Request power info
            time_start = steady_clock::now();
        }
    }

    // Shut down Teensy comms
}


// FLIR Camera Server
struct DeputyAuxServer {

    DeputyAuxServer();
    
    ~DeputyAuxServer();

string startServer(){
    string ret_msg;
	if(GLOB_DA_STATUS == 0){
		pthread_create(&GLOB_DA_THREAD, NULL, serverLoop, NULL);
		ret_msg = "Starting DA server";
	}else{
		ret_msg = "DA server already running";
	}
	return ret_msg;
}


string stopServer(){
	string ret_msg;
	if(GLOB_DA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_DA_STATUS = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);

		pthread_join(GLOB_DA_THREAD, NULL);
		ret_msg = "Disconnected Camera";

	}else{
		ret_msg = "DA server not currently running (or is connecting)";
	}

	return ret_msg;
}


string turnLEDOn(){
    string ret_msg;
	if(GLOB_DA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_DA_REQUEST = 2;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Turning LED on";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
	}
	return ret_msg;
}


string turnLEDOff(){
    string ret_msg;
	if(GLOB_DA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_DA_REQUEST = 3;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Turning LED off";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
	}
	return ret_msg;
}


powerStruct requestPower(){
	powerStruct ret_p;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    ret_p = GLOB_DA_POWER_VALUES;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_p;
	}
}

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

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<DeputyAuxServer>("DA")
        // To insterface a class method, you can use the `def` method.
        .def("connect", &DeputyAuxServer::startServer, "Start the server")
        .def("disconnect", &DeputyAuxServer::stopServer, "Stop the server")
        .def("LEDOn", &DeputyAuxServer::turnLEDOn, "Turn on the LED")
        .def("LEDOff", &DeputyAuxServer::turnLEDOff, "Turn off the LED")
        .def("reqpower", &DeputyAuxServer::requestPower, "Request power values")

}
