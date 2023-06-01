
#include <fmt/core.h>
#include <iostream>
#include <stdint.h>
#include <commander/commander.h>
#include <time.h>
#include <pthread.h>
#include "chiefAuxGlobals.hpp"
#include <cstdlib>
#include "SerialPort.h"

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

Comms::SerialPort teensy_port(128);

// Calculate differential voltage corresponding to the given displacement
double displacementToVoltage(double displacement){
    //Linear??
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    double a = GLOB_CA_TIPTILT_VOLTAGE_FACTOR;
	double b = GLOB_CA_TIPTILT_PIXEL_CONVERSION;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

    double result = a*b*displacement;
    
    return result;
}

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
				cout << "MOVING FINE STAGE BY " << GLOB_CA_FINESTAGE_STEPS << " STEPS AT " << GLOB_CA_FINESTAGE_FREQUENCY << " Hz, DIRECTION = " << GLOB_CA_FINESTAGE_DIRECTION << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
                break;
			case 2:
				// Move tip/tilt actuator Dextra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING DEXTRA X PIEZO TO " << GLOB_CA_TIPTILT_PIEZO_DEXTRA_X << " V" << endl;
				cout << "MOVING DEXTRA Y PIEZO TO " << GLOB_CA_TIPTILT_PIEZO_DEXTRA_Y << " V" << endl;
				pthread_mutex_unlock(&GLOB_DATA_LOCK);
		        GLOB_CA_REQUEST = 0;
				break;
			case 3:
				// Move tip/tilt actuator Sinistra
				pthread_mutex_lock(&GLOB_DATA_LOCK);
				cout << "MOVING SINISTRA X PIEZO TO " << GLOB_CA_TIPTILT_PIEZO_SINISTRA_X << " V" << endl;
				cout << "MOVING SINISTRA Y PIEZO TO " << GLOB_CA_TIPTILT_PIEZO_SINISTRA_Y << " V" << endl;
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


string moveFineStage(int numSteps, int direction, int freq){
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_FINESTAGE_STEPS = numSteps;
        GLOB_CA_FINESTAGE_DIRECTION = direction;
        GLOB_CA_FINESTAGE_FREQUENCY = freq;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 1;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving fine stage by " + to_string(numSteps) + " steps, at " + to_string(freq) + " Hz, direction = "+ to_string(direction);
	}else{
		ret_msg = "CA server not currently running (or is connecting)";
	}
	return ret_msg;
}


piezoXYStatus moveTipTiltDextra(double x_voltage, double y_voltage){
    piezoXYStatus ret_val;
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_TIPTILT_PIEZO_DEXTRA_X = x_voltage;
		GLOB_CA_TIPTILT_PIEZO_DEXTRA_Y = y_voltage;
		ret_val.X = GLOB_CA_TIPTILT_PIEZO_DEXTRA_X;
		ret_val.Y = GLOB_CA_TIPTILT_PIEZO_DEXTRA_Y;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 2;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Dextra to X = " + to_string(x_voltage) + " V, Y = " + to_string(y_voltage) + " V";
	}else{
	    ret_val.X = 0;
		ret_val.Y = 0;
		ret_msg = "CA server not currently running (or is connecting)";
	}
    ret_val.msg = ret_msg;
	return ret_val;
}

piezoXYStatus moveTipTiltSinistra(double dx_voltage, double dy_voltage){
    piezoXYStatus ret_val;
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_TIPTILT_PIEZO_SINISTRA_X = dx_voltage;
		GLOB_CA_TIPTILT_PIEZO_SINISTRA_Y = dy_voltage;
		ret_val.X = GLOB_CA_TIPTILT_PIEZO_SINISTRA_X;
		ret_val.Y = GLOB_CA_TIPTILT_PIEZO_SINISTRA_Y;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 3;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving tip/tilt piezo Sinistra to X = " + to_string(dx_voltage) + " V, Y = " + to_string(dy_voltage) + " V";
	}else{
	    ret_val.X = 0;
		ret_val.Y = 0;
		ret_msg = "CA server not currently running (or is connecting)";
	}
    ret_val.msg = ret_msg;
	return ret_val;
}

piezoStatus moveSciPiezo(double voltage){
    piezoStatus ret_val;
    string ret_msg;
	if(GLOB_CA_STATUS==2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);

		pthread_mutex_lock(&GLOB_DATA_LOCK);
		GLOB_CA_SCIPIEZO_VOLTAGE = voltage;
		ret_val.voltage = GLOB_CA_SCIPIEZO_VOLTAGE;
		pthread_mutex_unlock(&GLOB_DATA_LOCK);

		GLOB_CA_REQUEST = 4;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);
		ret_msg = "Moving science piezo to " + to_string(voltage) + " V";
	}else{
	    ret_val.voltage = 0;
		ret_msg = "CA server not currently running (or is connecting)";
	}
	ret_val.msg = ret_msg;
	return ret_val;
}


powerStatus requestPower(){
    powerStatus ret_p;
	powerStruct p;
    string ret_msg;
    
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    p = GLOB_CA_POWER_VALUES;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    
    ret_p.current = p.current;
    ret_p.voltage = p.voltage;
    
    ret_msg = "Voltage = " + to_string(p.voltage) + ", Current = " + to_string(p.current);
    
    ret_p.msg = ret_msg;
    
    return ret_p;
}

string receiveTipTiltPos(centroid Dpos, centroid Spos){
	double DextraDx = displacementToVoltage(Dpos.x) + CURRENT_DEXTRA_X_VOLTAGE;
	double DextraDy = displacementToVoltage(Dpos.y) + CURRENT_DEXTRA_Y_VOLTAGE;
	double SinistraDx = displacementToVoltage(Spos.x) + CURRENT_SINISTRA_X_VOLTAGE;
	double SinistraDy = displacementToVoltage(Spos.y) + CURRENT_SINISTRA_Y_VOLTAGE;

	// SEND TO TEENSY


}

void test_piezo(uint8_t zero, uint8_t one, uint8_t two, uint8_t three, uint8_t four) {
    teensy_port.piezo_duties[0] = zero;
    teensy_port.piezo_duties[1] = one;
    teensy_port.piezo_duties[2] = two;
    teensy_port.piezo_duties[3] = three;
    teensy_port.piezo_duties[4] = four;
    teensy_port.Request(SETPWM);
    teensy_port.SendAllRequests();
}

void test_watt() {
    teensy_port.Request(WATTMETER);
	teensy_port.SendAllRequests();
	usleep(150);
	teensy_port.ReadMessage();
	cout << "PC Voltage (mV): " << teensy_port.PC_Voltage << endl;
	cout << "PC Current (mA): " << teensy_port.PC_Current << endl;
	cout << "Motor Voltage (mV): " << teensy_port.Motor_Voltage << endl;
	cout << "Motor Current (mA): " << teensy_port.Motor_Current << endl;
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

    template <>
    struct adl_serializer<piezoXYStatus> {
        static void to_json(json& j, const piezoXYStatus& p) {
            j = json{{"X_voltage", p.X}, {"Y_voltage", p.Y}, {"message", p.msg}};
        }

        static void from_json(const json& j, piezoXYStatus& p) {
            j.at("X_voltage").get_to(p.X);
            j.at("Y_voltage").get_to(p.Y);
            j.at("message").get_to(p.msg);
        }
    };
    
    template <>
    struct adl_serializer<piezoStatus> {
        static void to_json(json& j, const piezoStatus& p) {
            j = json{{"voltage", p.voltage}, {"message", p.msg}};
        }

        static void from_json(const json& j, piezoStatus& p) {
            j.at("voltage").get_to(p.voltage);
            j.at("message").get_to(p.msg);
        }
    };

    template <>
    struct adl_serializer<centroid> {
        static void to_json(json& j, const centroid& c) {
            j = json{{"x", c.x},{"y", c.y}};
        }

        static void from_json(const json& j, centroid& c) {
            j.at("x").get_to(c.x);
            j.at("y").get_to(c.y);
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
		.def("scipiezo", &ChiefAuxServer::moveSciPiezo, "Move science piezo")
		.def("receiveTipTiltPos", &ChiefAuxServer::receiveTipTiltPos, "Receive positions to move tip tilt piezos")
		.def("testpiezo", &ChiefAuxServer::test_piezo, "testing")
		.def("testwatt", &ChiefAuxServer::test_watt, "testing");

}
