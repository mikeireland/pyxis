#include <fmt/core.h>
#include <iostream>
#include <stdint.h>
#include <commander/commander.h>
#include <time.h>
#include <cstdlib>
#include "SerialPort.h"
#include "chiefAuxGlobals.hpp"

using json = nlohmann::json;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

using namespace std;

Comms::SerialPort teensy_port(127);

struct piezoPWMvals{
    double DextraX_V = -12.0;
    double DextraY_V = -12.0;
    double SinistraX_V = -12.0;
    double SinistraY_V = -12.0;
    double Science_V =  -12.0;
    
    double DextraX_um = 0.0;
    double DextraY_um = 0.0;
    double SinistraX_um = 0.0;
    double SinistraY_um = 0.0;
    double Science_um = 0.0;
};

struct powerStatus{
    double PC_V;
    double PC_A;
    double motor_V;
    double motor_A;
};

struct piezoStatus{
    double um;
    double voltage;
    string msg;
};

struct status{
    piezoPWMvals ppv;
    int32_t sdc_step_count;
    powerStatus ps;
    string msg;
};

struct centroid {
    double x;
    double y;
};

piezoPWMvals PPV;
powerStatus PS;
double SDC_pos;
int32_t SDC_step_count;

double V_max = 100.0;
double V_min = -12.0;
double V_diff = V_max - V_min;


// FLIR Camera Server
struct ChiefAuxServer {

    ChiefAuxServer(){
        fmt::print("ChiefAuxServer\n");
        
    }

    ~ChiefAuxServer(){
        fmt::print("~ChiefAuxServer\n");
    }

    status requestStatus(){
        status ret_status;
        
        ret_status.ppv = PPV;
        readWattmeterAndSDC();
        ret_status.ps = PS;
        ret_status.sdc_step_count = SDC_step_count;

        ret_status.msg = "Updated values";

        return ret_status;
    }

    string moveSDC(int32_t steps, uint16_t period) {
        string ret_msg;
        // -ve steps is towards middle of injection system
        int32_to_bytes(steps, &teensy_port.steps[0], &teensy_port.steps[1], &teensy_port.steps[2], &teensy_port.steps[3]);
        uint16_to_bytes(period, &teensy_port.period[0], &teensy_port.period[1]);
        teensy_port.Request(SETSDC);
        teensy_port.SendAllRequests();
        ret_msg = "Moving fine stage " + to_string(steps) + " at " + to_string(period) + " microseconds per step";
        return ret_msg;
    }

    string homeSDC() {
        string ret_msg;
        // -ve steps is towards middle of injection system
        int32_t steps = 450000;
        uint16_t period = 100;
        int32_to_bytes(steps, &teensy_port.steps[0], &teensy_port.steps[1], &teensy_port.steps[2], &teensy_port.steps[3]);
        uint16_to_bytes(period, &teensy_port.period[0], &teensy_port.period[1]);
        teensy_port.Request(SETSDC);
        teensy_port.SendAllRequests();

        while (true){
            sleep(1);
            teensy_port.Request(GETSDC);
            teensy_port.SendAllRequests();
            usleep(150);
            teensy_port.ReadMessage();
            if (teensy_port.current_step == 0){
                break;
            }       
        }

        ret_msg = "Fine stage is homed";
        //WAIT TO RETURN???
        return ret_msg;
    }

    piezoStatus moveTipTiltPiezo(int flag, double voltage){
        
        piezoStatus ps;

        if (voltage < V_max && voltage >= V_min){
            
            //double um_to_V = 0.7614213197969543;
            //double voltage = um_to_V*um-15;
            double V_to_um = 1.313333333;

            double um = V_to_um*(voltage - V_min);

            string ret_msg_tmp; 

            if (flag == 0){
                PPV.DextraX_V = voltage;
                PPV.DextraX_um = um;
                ret_msg_tmp = "Dextra X";
            } else if (flag == 1){
                PPV.DextraY_V = voltage;
                PPV.DextraY_um = um;
                ret_msg_tmp = "Dextra Y";
            } else if (flag == 2){
                PPV.SinistraX_V = voltage;
                PPV.SinistraX_um = um;
                ret_msg_tmp = "Sinistra X";
            } else if (flag == 3){
                PPV.SinistraY_V = voltage;
                PPV.SinistraY_um = um;
                ret_msg_tmp = "Sinistra Y";
            }

            sendPiezoVals(PPV);
            string ret_msg = "Moved " + ret_msg_tmp + " piezo to " + to_string(voltage) + "V (" + to_string(um) + " microns)";
            
            ps.um = um;
            ps.voltage = voltage;
            ps.msg = ret_msg;

        } else{
            ps.um = 0.0;
            ps.voltage = 0.0;
            ps.msg = "Piezo value out of range";
        }
        
        return ps;
    }

    
    string receiveRelativeTipTiltPos(centroid Dpos, centroid Spos, double gain){

        string ret_msg;
        cout << Dextra_angle << endl;
        double px_to_um = 1.725; 
        double um_to_V = 0.7614213197969543;
        double conversion = px_to_um*um_to_V*gain;
        
        double Dx = cos(Dextra_angle)*Dpos.x - sin(Dextra_angle)*Dpos.y;
        double Dy = -sin(Dextra_angle)*Dpos.x - cos(Dextra_angle)*Dpos.y;
        double Sx = cos(Sinistra_angle)*Spos.x - sin(Sinistra_angle)*Spos.y;
        double Sy = sin(Sinistra_angle)*Spos.x + cos(Sinistra_angle)*Spos.y;

        cout << Dpos.x << endl;
        cout << Dpos.y << endl;
        cout << Dx << endl;

        PPV.DextraX_um += px_to_um*Dx;
        PPV.DextraY_um += px_to_um*Dy;
        PPV.SinistraX_um += px_to_um*Sx;
        PPV.SinistraY_um += px_to_um*Sy;

        PPV.DextraX_V += conversion*Dx;
        PPV.DextraY_V += conversion*Dy;
        PPV.SinistraX_V += conversion*Sx;
        PPV.SinistraY_V += conversion*Sy;
        
        if (PPV.DextraX_V > V_max){
            PPV.DextraX_V = V_max;
        } else if (PPV.DextraX_V < V_min){
            PPV.DextraX_V = V_min;
        }
        
        if (PPV.DextraY_V > V_max){
            PPV.DextraY_V = V_max;
        } else if (PPV.DextraY_V < V_min){
            PPV.DextraY_V = V_min;
        }
        
        if (PPV.SinistraX_V > V_max){
            PPV.SinistraX_V = V_max;
        } else if (PPV.SinistraX_V < V_min){
            PPV.SinistraX_V = V_min;
        }
        
        if (PPV.SinistraY_V > V_max){
            PPV.SinistraY_V = V_max;
        } else if (PPV.SinistraY_V < V_min){
            PPV.SinistraY_V = V_min;
        }
        sendPiezoVals(PPV);
        
        ret_msg = to_string(conversion*Dx) + " dx, " + to_string(conversion*Dy) + " dy, " + to_string(conversion*Sx) + " sx, " + to_string(conversion*Sy) + " sy";
        
        return ret_msg;
    }

    piezoStatus moveSciPiezo(double voltage){

        piezoStatus ps;

        if (voltage < V_max && voltage >= V_min){
            //double img_px_to_img_um = 6.5;
            //double img_um_to_piezo_um = 0.8333333333;
            //double um_to_V = 6.428801028608165;
            //double conversion = img_px_to_img_um*img_um_to_piezo_um*um_to_V;
            // double im_px_to_piezo_um = 5.41666666;
            double V_to_um = 0.15555;
            
            double um = V_to_um*(voltage - V_min);
            PPV.Science_V = voltage;
            PPV.Science_um = um;

            string ret_msg = "Moved science piezo to " + to_string(PPV.Science_V) + "V (moved " + to_string(um) + " microns)";
            
            sendPiezoVals(PPV);

            ps.um = um;
            ps.voltage = voltage;
            ps.msg = ret_msg;
        
        } else {
            ps.um = 0.0;
            ps.voltage = 0.0;
            ps.msg = "Piezo value out of range";
        }
        return ps;
    }

    int32_t getSDCpos(){
        teensy_port.Request(GETSDC);
        teensy_port.SendAllRequests();
        usleep(150);
        teensy_port.ReadMessage();
        SDC_step_count = -teensy_port.current_step;
        return SDC_step_count;
    }

    void readWattmeterAndSDC() {
        teensy_port.Request(WATTMETER);
        teensy_port.Request(GETSDC);
        teensy_port.SendAllRequests();
        usleep(150);
        teensy_port.ReadMessage();
        
        PS.PC_V = teensy_port.PC_Voltage;
        PS.PC_A = teensy_port.PC_Current;
        PS.motor_V = teensy_port.Motor_Voltage;
        PS.motor_A = teensy_port.Motor_Current;
        
        SDC_step_count = -teensy_port.current_step;

        cout << "PC Voltage (mV): " << PS.PC_V << endl;
        cout << "PC Current (mA): " << PS.PC_A << endl;
        cout << "Motor Voltage (mV): " << PS.motor_V << endl;
        cout << "Motor Current (mA): " << PS.motor_A << endl;
        cout << "SDC position (um): " << SDC_step_count << endl;
    }

    uint8_t roundPWM(double voltage){
        uint8_t ret_val;
        double V_to_PWM = 256/(V_max - V_min);
        long temp = lround(V_to_PWM*(voltage-V_min));
        ret_val = (uint8_t) temp;
        return temp;
    }

    void sendPiezoVals(piezoPWMvals ppv) {
        teensy_port.piezo_duties[0] = roundPWM(ppv.DextraX_V);
        teensy_port.piezo_duties[1] = roundPWM(ppv.DextraY_V);
        teensy_port.piezo_duties[2] = roundPWM(ppv.SinistraX_V);
        teensy_port.piezo_duties[3] = roundPWM(ppv.SinistraY_V);
        teensy_port.piezo_duties[4] = roundPWM(ppv.Science_V);
        teensy_port.Request(SETPWM);
        teensy_port.SendAllRequests();
    }

/*
    string moveHV(int flag, double voltageX, double voltageY){
        
        piezoStatus ps;
        double voltageXd, voltageYd;
        if (flag == 0){
            voltageXd = cos(Dextra_angle)*voltageX + sin(Dextra_angle)*voltageY;
            PPV.DextraX_V += voltageXd;
            voltageYd = -sin(Dextra_angle)*voltageX + cos(Dextra_angle)*voltageY;
            PPV.DextraY_V += voltageYd;
        } else if (flag == 1){
            voltageXd = cos(Sinistra_angle)*voltageX + sin(Sinistra_angle)*voltageY;
            PPV.SinistraX_V += voltageXd;
            voltageYd = sin(Sinistra_angle)*voltageX - cos(Sinistra_angle)*voltageY;
            PPV.SinistraY_V += voltageYd;
        }
        
        cout << voltageXd << ", " << voltageYd << endl;
        cout << Dextra_angle << endl;
        //sendPiezoVals(PPV);
        
        double voltage = sqrt(voltageXd*voltageXd + voltageYd*voltageYd);
       
        return to_string(voltage);
    }

    string testservo(double dx, double dy, double sx, double sy){

        centroid Dpos, Spos;
        Dpos.x = dx;
        Dpos.y = dy;
        Spos.x = sx;
        Spos.y = sy;

        string ret_msg;
        cout << Dextra_angle << endl;
        double px_to_um = 1.725; 
        double um_to_V = 0.7614213197969543;
        double conversion = px_to_um*um_to_V;
        
        double Dx = cos(Dextra_angle)*Dpos.x - sin(Dextra_angle)*Dpos.y;
        double Dy = -sin(Dextra_angle)*Dpos.x - cos(Dextra_angle)*Dpos.y;
        double Sx = cos(Sinistra_angle)*Spos.x - sin(Sinistra_angle)*Spos.y;
        double Sy = sin(Sinistra_angle)*Spos.x + cos(Sinistra_angle)*Spos.y;

        cout << Dpos.x << endl;
        cout << Dpos.y << endl;
        cout << Dx << endl;

        PPV.DextraX_um += px_to_um*Dx;
        PPV.DextraY_um += px_to_um*Dy;
        PPV.SinistraX_um += px_to_um*Sx;
        PPV.SinistraY_um += px_to_um*Sy;

        PPV.DextraX_V += conversion*Dx;
        PPV.DextraY_V += conversion*Dy;
        PPV.SinistraX_V += conversion*Sx;
        PPV.SinistraY_V += conversion*Sy;
        
        if (PPV.DextraX_V > V_max){
            PPV.DextraX_V = V_max;
        } else if (PPV.DextraX_V < V_min){
            PPV.DextraX_V = V_min;
        }
        
        if (PPV.DextraY_V > V_max){
            PPV.DextraY_V = V_max;
        } else if (PPV.DextraY_V < V_min){
            PPV.DextraY_V = V_min;
        }
        
        if (PPV.SinistraX_V > V_max){
            PPV.SinistraX_V = V_max;
        } else if (PPV.SinistraX_V < V_min){
            PPV.SinistraX_V = V_min;
        }
        
        if (PPV.SinistraY_V > V_max){
            PPV.SinistraY_V = V_max;
        } else if (PPV.SinistraY_V < V_min){
            PPV.SinistraY_V = V_min;
        }
        
        sendPiezoVals(PPV);
        
        ret_msg = to_string(conversion*Dx) + " dx, " + to_string(conversion*Dy) + " dy, " + to_string(conversion*Sx) + " sx, " + to_string(conversion*Sy) + " sy";
        
        return ret_msg;
    }*/

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
                     {"Dextra_X_V", s.ppv.DextraX_V}, 
                     {"Dextra_Y_V", s.ppv.DextraY_V},
                     {"Sinistra_X_V", s.ppv.SinistraX_V}, 
                     {"Sinistra_Y_V", s.ppv.SinistraY_V},
                     {"Science_V", s.ppv.Science_V},
                     {"Dextra_X_um", s.ppv.DextraX_um}, 
                     {"Dextra_Y_um", s.ppv.DextraY_um},
                     {"Sinistra_X_um", s.ppv.SinistraX_um}, 
                     {"Sinistra_Y_um", s.ppv.SinistraY_um},
                     {"Science_um", s.ppv.Science_um},
                     {"SDC_step_count", s.sdc_step_count},
                     {"message",s.msg}};
        }

        static void from_json(const json& j, status& s) {
            j.at("PC_current").get_to(s.ps.PC_A);
            j.at("PC_voltage").get_to(s.ps.PC_V);
            j.at("Motor_current").get_to(s.ps.motor_A);
            j.at("Motor_voltage").get_to(s.ps.motor_V);
            j.at("Dextra_X_V").get_to(s.ppv.DextraX_V);
            j.at("Dextra_Y_V").get_to(s.ppv.DextraY_V);
            j.at("Sinistra_X_V").get_to(s.ppv.SinistraX_V);
            j.at("Sinistra_Y_V").get_to(s.ppv.SinistraY_V);
            j.at("Science_V").get_to(s.ppv.Science_V);
            j.at("Dextra_X_um").get_to(s.ppv.DextraX_um);
            j.at("Dextra_Y_um").get_to(s.ppv.DextraY_um);
            j.at("Sinistra_X_um").get_to(s.ppv.SinistraX_um);
            j.at("Sinistra_Y_um").get_to(s.ppv.SinistraY_um);
            j.at("Science_um").get_to(s.ppv.Science_um);
            j.at("SDC_step_count").get_to(s.sdc_step_count);
            j.at("message").get_to(s.msg);
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

    template <>
    struct adl_serializer<piezoStatus> {
        static void to_json(json& j, const piezoStatus& ps) {
            j = json{{"voltage", ps.voltage},{"position", ps.um},{"message", ps.msg}};
        }

        static void from_json(const json& j, piezoStatus& ps) {
            j.at("voltage").get_to(ps.voltage);
            j.at("position").get_to(ps.um);
            j.at("message").get_to(ps.msg);
        }
    };
}

// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<ChiefAuxServer>("CA")
        // To insterface a class method, you can use the `def` method.
        .def("requestStatus", &ChiefAuxServer::requestStatus, "Get information on all actuators and power")
		.def("moveSDC", &ChiefAuxServer::moveSDC, "Move fine stage")
        .def("homeSDC", &ChiefAuxServer::homeSDC, "Home fine stage")
        .def("SDCpos", &ChiefAuxServer::getSDCpos, "Get fine stage position")
        //.def("test", &ChiefAuxServer::testservo, "Home fine stage")
		.def("moveTipTiltPiezo", &ChiefAuxServer::moveTipTiltPiezo, "Set the tip/tilt piezos")
		.def("receiveRelativeTipTiltPos", &ChiefAuxServer::receiveRelativeTipTiltPos, "Receive positions to move tip tilt piezos")
		//.def("moveHV", &ChiefAuxServer::moveHV, "Move piezos horizontally and vertically")
		.def("moveSciPiezo", &ChiefAuxServer::moveSciPiezo, "Set the science piezos");
}
