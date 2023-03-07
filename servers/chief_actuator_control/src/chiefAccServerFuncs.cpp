
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
				usleep(10);
				break;
			case 2:
				// Move fringe tracking stage
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				GLOB_CA_FINESTAGE_STEPS
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
                break;
			case 3:
				// Move tip/tilt actuator Dextra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				GLOB_CA_TIPTILT_PIEZO_DEXTRA
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
				break;
			case 4:
				// Move tip/tilt actuator Sinistra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				GLOB_CA_TIPTILT_PIEZO_SINISTRA
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
                break;
			case 5:
				// Move science stage piezo
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				GLOB_CA_SCIPIEZO_VOLTAGE
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
				break;
			default: //if no correct flag, turn off
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
    	        GLOB_CA_STATUS = 0;
		        pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				break;
		}

		time_current = steady_clock::now();

		auto delta_time = duration_cast<milliseconds>(time_current-time_start).count();

        if(delta_time>GLOB_CA_POWER_REQUEST_TIME){

            // Request power info
            time_start = steady_clock::now();
        }
    }

    // Shut down Teensy comms
}


// FLIR Camera Server
struct ChiefAccServer {

    ChiefAccServer();
    
    ~ChiefAccServer();


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

string moveFineStage(int numSteps){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_FINESTAGE_STEPS = numSteps;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 2;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving fine stage";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
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

		GLOB_CA_REQUEST = 3;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Dextra";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
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

		GLOB_CA_REQUEST = 4;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Sinistra";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
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

		GLOB_CA_REQUEST = 1;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving science piezo";
	}else{
		ret_msg = "DA server not currently running (or is connecting)";
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
    m.instance<ChiefAccServer>("CA")
        // To insterface a class method, you can use the `def` method.
        .def("connect", &ChiefAccServer::startServer, "Start the server")
        .def("disconnect", &ChiefAccServer::stopServer, "Stop the server")
        .def("reqpower", &ChiefAccServer::requestPower, "Request power values")
		.def("finestage", &ChiefAccServer::moveFineStage, "Move fine stage")
		.def("tiptiltD", &ChiefAccServer::moveTipTiltDextra, "Move tip tilt piezo for Dextra beam")
		.def("tiptiltS", &ChiefAccServer::moveTipTiltSinistra, "Move tip tilt piezo for Sinistra beam")
		.def("scipiezo", &ChiefAccServer::moveSciPiezo, "Move science piezo")

}
