
#include <fmt/core.h>
#include <iostream>
#include <commander/commander.h>
#include <time.h>
#include <pthread.h>
#include "chiefAuxGlobals.hpp"
#include <cstdlib>

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

void* serverLoop(void*){

    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CA_STATUS = 1; // Connecting
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

    auto time_start = steady_clock::now();
    auto time_current = steady_clock::now();
    //START UP TEENSY COMMS

    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CA_STATUS = 2; // Running/waiting
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);  

    time_start = steady_clock::now();
    while(GLOB_CA_STATUS>0){

        switch(GLOB_CA_REQUEST) {
			case 1:
				// Move fringe tracking stage
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING FINE STAGE BY " << GLOB_CA_FINESTAGE_STEPS << " STEPS" << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
                break;
			case 2:
				// Move tip/tilt actuator Dextra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING DEXTRA PIEZO BY " << GLOB_CA_TIPTILT_PIEZO_DEXTRA << " V" << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
				break;
			case 3:
				// Move tip/tilt actuator Sinistra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING SINISTRA PIEZO BY " << GLOB_CA_TIPTILT_PIEZO_SINISTRA << " V" << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
                break;
			case 4:
				// Move science stage piezo
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING SCIENCE PIEZO BY " << GLOB_CA_SCIPIEZO_VOLTAGE << " V" << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
				break;
			default: //if no correct flag, sleep
                usleep(1000);
				break;
		}

		time_current = steady_clock::now();

		auto delta_time = duration_cast<milliseconds>(time_current-time_start).count();

        if(delta_time>GLOB_CA_POWER_REQUEST_TIME){

            // Request power info
            powerStruct result;
            result.current = rand() % 100;
            result.voltage = rand() % 100;
            
            pthread_mutex_lock(&GLOB_FLAG_LOCK);
            GLOB_CA_POWER_VALUES = result;
            pthread_mutex_unlock(&GLOB_FLAG_LOCK);
            
            time_start = steady_clock::now();
        }
    }

    pthread_exit(NULL);
    // Shut down Teensy comms
}


// FLIR Camera Server
struct ChiefAuxServer {


    ChiefAuxServer()
    {
        fmt::print("ChiefAuxServer\n");
    }

    ~ChiefAuxServer()
    {
        fmt::print("~ChiefAuxServer\n");
    }


string startServer(){
    string ret_msg;
	if(GLOB_CA_STATUS == 0){
		pthread_create(&GLOB_CA_THREAD, NULL, serverLoop, NULL);
		ret_msg = "Starting CA server";
	}else{
		ret_msg = "CA server already running";
	}
	return ret_msg;
}

//Get status
string status(){
	string ret_msg;
	if(GLOB_CA_STATUS == 0){
		ret_msg = "Server Not Connected!";
	}else if(GLOB_CA_STATUS == 1){
		ret_msg = "Server Connecting";
	}else{
		ret_msg = "Server Running";
	}

	return ret_msg;
}

string stopServer(){
	string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_CA_STATUS = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);

		pthread_join(GLOB_CA_THREAD, NULL);
		ret_msg = "Stopped Server";

	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}

	return ret_msg;
}

string moveFineStage(int numSteps){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_FINESTAGE_STEPS = numSteps;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 1;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving fine stage " + to_string(numSteps) + " steps";
	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}
	return ret_msg;
}


string moveTipTiltDextra(double voltage){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_TIPTILT_PIEZO_DEXTRA = voltage;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 2;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Dextra by " + to_string(voltage) + " V";;
	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}
	return ret_msg;
}

string moveTipTiltSinistra(double voltage){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_TIPTILT_PIEZO_SINISTRA = voltage;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 3;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Sinistra by " + to_string(voltage) + " V";
	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}
	return ret_msg;
}



string moveSciPiezo(double voltage){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_SCIPIEZO_VOLTAGE = voltage;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);
		

		GLOB_CA_REQUEST = 4;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving science piezo by " + to_string(voltage) + " V";
	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}
	return ret_msg;
}


powerStruct requestPower(){
	powerStruct ret_p;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    ret_p = GLOB_CA_POWER_VALUES;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
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

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<ChiefAuxServer>("CA")
        // To insterface a class method, you can use the `def` method.
        .def("connect", &ChiefAuxServer::startServer, "Start the server")
        .def("disconnect", &ChiefAuxServer::stopServer, "Stop the server")
        .def("reqpower", &ChiefAuxServer::requestPower, "Request power values")
        .def("status", &ChiefAuxServer::status, "Server status")
		.def("finestage", &ChiefAuxServer::moveFineStage, "Move fine stage")
		.def("tiptiltD", &ChiefAuxServer::moveTipTiltDextra, "Move tip tilt piezo for Dextra beam")
		.def("tiptiltS", &ChiefAuxServer::moveTipTiltSinistra, "Move tip tilt piezo for Sinistra beam")
		.def("scipiezo", &ChiefAuxServer::moveSciPiezo, "Move science piezo");

}
