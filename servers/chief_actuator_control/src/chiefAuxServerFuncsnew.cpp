
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

struct piezoPWMvals{
    uint8_t DextraX;
    uint8_t DextraY;
    uint8_t SinistraX;
    uint8_t SinistraY;
    uint8_t Science;
}

struct powerStatus{
    double PC_V;
    double PC_A;
    double motor_V;
    double motor_A;
}

struct status{
    piezoPWMvals ppv;
    int stage_pos;
    double stage_freq;
    powerStatus ps;
}


piezoPWMvals PPV;
powerStatus PS;

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


string moveFineStage(int numSteps, int freq){
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


status requestStatus(){
    status ret_status;
    
    status.ppv = PPV;
    readWattmeter();
    status.ps = PS;
    
    //Science SDC STAGE
    status.XXX

    return ret_status;
}

string moveTipTiltPiezos(double Dx_um, double Dy_um, double Sx_um, double Sy_um){
	string ret_msg;
	if ((Dx_um < 151 && Dx_um >= 0) && (Dy_um < 151 && Dy_um >= 0) && (Sx_um < 151 && Sx_um >= 0) && (Sy_um < 151 && Sy_um >= 0)){
        //double um_to_V = 0.7614213197969543;
        //double V_to_PWM = 2.217391304347826;
        //double conversion = um_to_V*V_to_PWM;
        double conversion = 1.6883690134628118;
	    
	    PPV.DextraX = round(conversion*Dx_um);
	    PPV.DextraY = round(conversion*Dy_um);
	    PPV.SinistraX = round(conversion*Sx_um);
	    PPV.SinistraY = round(conversion*Sy_um);
	    
        string ret_msg_a = "Moved Dextra piezo to " + to_string(DextraX) + ", " + to_string(DextraY) + "(" + to_string(Dx_um) + ", "+ to_string(Dy_um) " microns)";
        string ret_msg_b = "Moved Sinistra piezo to " + to_string(SinistraX) + ", " + to_string(SinistraY) + "(" + to_string(Sx_um) + ", "+ to_string(Sy_um) " microns)";
	    
	    sendPiezoVals(PPV);
	    ret_msg = ret_msg_a + " and " + ret_msg_b;
	    
	} else{
	    ret_msg = "Piezo values out of range";
	}
	
	return ret_msg
}

int receiveRelativeTipTiltPos(centroid Dpos, centroid Spos){

    //double px_to_um = 1.725; 
    //double um_to_V = 0.7614213197969543;
    //double V_to_PWM = 2.217391304347826;
    //double conversion = px_to_um*um_to_V*V_to_PWM
    double conversion = 2.9124365482233503;
    
	PPV.DextraX += round(conversion*Dpos.x);
	PPV.DextraY += round(conversion*Dpos.y);
	PPV.SinistraX += round(conversion*Spos.x);
	PPV.SinistraY += round(conversion*Spos.y);
	
	sendPiezoVals(PPV);
	
	return 0;
}

string moveSciPiezo(double img_px){

    string ret_msg;
    if (img_px < XX && img_px >= 0){
        //double img_px_to_img_um = 6.5;
        //double img_um_to_piezo_um = 0.8333333333;
        //double um_to_V = ;
        //double V_to_PWM = ;
	    //double conversion = img_px_to_img_um*img_um_to_piezo_um*um_to_V*V_to_PWM;
	    double conversion = ;
	    
	    PPV.science = round(img_px*conversion);
	    
	    ret_msg = "Moved science piezo to " + to_string(PPV.science) + "(moved " + to_string(img_px*img_px_to_img_um*img_um_to_piezo_um) + " microns)";
	    
	    sendPiezoVals(PPV);
    
    } else {
        ret_msg = "Piezo value out of range";
    }
	return ret_msg;
}

void readWattmeter() {
    teensy_port.Request(WATTMETER);
	teensy_port.SendAllRequests();
	usleep(150);
	teensy_port.ReadMessage();
	
	PS.PC_V = teensy_port.PC_Voltage;
	PS.PC_A = teensy_port.PC_Current;
	PS.motor_V = teensy_port.Motor_Voltage;
	PS.motor_A = teensy_port.Motor_Current;
	
	cout << "PC Voltage (mV): " << PS.PC_V << endl;
	cout << "PC Current (mA): " << PS.PC_A << endl;
	cout << "Motor Voltage (mV): " << PS.motor_V << endl;
	cout << "Motor Current (mA): " << teensy_port.Motor_Current << endl;
}

void sendPiezoVals(piezoPWMvals ppv) {
    teensy_port.piezo_duties[0] = ppv.DextraX;
    teensy_port.piezo_duties[1] = ppv.DextraY;
    teensy_port.piezo_duties[2] = ppv.SinistraX;
    teensy_port.piezo_duties[3] = ppv.SinistraY;
    teensy_port.piezo_duties[4] = ppv.science;
    teensy_port.Request(SETPWM);
    teensy_port.SendAllRequests();
}

};

// Serialiser to convert configuration struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<status> {
        static void to_json(json& j, const status& s) {
            j = json{{"PC_current", s.ps.PC_A}, 
                     {"PC_voltage", s.ps.PC_V},
                     {"Motor_current", s.ps.motor_A}, 
                     {"Motor_voltage", s.ps.motor_V},
                     {"Dextra X", s.ppv.DextraX}, 
                     {"Dextra Y", s.ppv.DextraY},
                     {"Sinistra X", s.ppv.SinistraX}, 
                     {"Sinistra Y", s.ppv.SinistraY},
                     {"Science", s.ppv.science},
                     {"SDC_step_count", XXX},
                     {"SDC_frequency", XXX}};
        }

        static void from_json(const json& j, powerStruct& p) {
            j.at("PC_current").get_to(s.ps.PC_A);
            j.at("PC_voltage").get_to(s.ps.PC_V);
            j.at("Motor_current").get_to(s.ps.motor_A);
            j.at("Motor_voltage").get_to(s.ps.motor_V);
            j.at("Dextra X").get_to(s.ppv.DextraX);
            j.at("Dextra Y").get_to(s.ppv.DextraY);
            j.at("Sinistra X").get_to(s.ppv.SinistraX);
            j.at("Sinistra Y").get_to(s.ppv.SinistraY);
            j.at("Science").get_to(s.ppv.science);
            j.at("SDC_step_count").get_to(XXX);
            j.at("SDC_frequency").get_to(XXX);
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
        .def("requestStatus", &ChiefAuxServer::requestStatus, "Get information on all actuators and power")
		.def("moveFineStage", &ChiefAuxServer::moveFineStage, "Move fine stage")
		.def("moveTipTiltPiezos", &ChiefAuxServer::moveTipTiltPiezos, "Set the tip/tilt piezos")
		.def("receiveRelativeTipTiltPos", &ChiefAuxServer::receiveRelativeTipTiltPos, "Receive positions to move tip tilt piezos")
		.def("moveSciPiezo", &ChiefAuxServer::moveSciPiezo, "Set the science piezos")
		.def("readWattmeter", &ChiefAuxServer::readWattmeter, "Get power information");

}
