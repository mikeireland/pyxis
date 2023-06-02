#include <fmt/core.h>
#include <iostream>
#include <stdint.h>
#include <commander/commander.h>
#include <time.h>
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
    double DextraX;
    double DextraY;
    double SinistraX;
    double SinistraY;
    double Science;
}

struct powerStatus{
    double PC_V;
    double PC_A;
    double motor_V;
    double motor_A;
}

struct piezoStatus{
    double um;
    double voltage;
    string msg;
}

struct status{
    piezoPWMvals ppv;
    double sdc_pos;
    powerStatus ps;
    string msg;
}

piezoPWMvals PPV;
powerStatus PS;
double SDC_pos;

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
        
        status.ppv = PPV;
        readWattmeterAndSDC();
        status.ps = PS;
        status.sdc_pos = SDC_pos;

        status.msg = "Updated values";

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

    piezoStatus moveTipTiltPiezos(int flag, double um){
        
        piezoStatus ps;

        if (um < 151 && um >= 0){
            
            double um_to_V = 0.7614213197969543;
            
            double voltage = um_to_V*um-15;

            string ret_msg_tmp; 

            if (flag == 0){
                PPV.DextraX = voltage;
                ret_msg_tmp = "Dextra X"
            } else if (flag == 1){
                PPV.DextraY = voltage;
                ret_msg_tmp = "Dextra Y"
            } else if (flag == 2){
                PPV.SinistraX = voltage;
                ret_msg_tmp = "Sinistra X"
            } else if (flag == 3){
                PPV.SinistraY = voltage;
                ret_msg_tmp = "Sinistra Y"
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


    string moveAllTipTiltPiezos(double Dx_um, double Dy_um, double Sx_um, double Sy_um){
        string ret_msg;
        if ((Dx_um < 151 && Dx_um >= 0) && (Dy_um < 151 && Dy_um >= 0) && (Sx_um < 151 && Sx_um >= 0) && (Sy_um < 151 && Sy_um >= 0)){
            
            double um_to_V = 0.7614213197969543;
            
            PPV.DextraX = um_to_V*Dx_um-15;
            PPV.DextraY = um_to_V*Dy_um-15;
            PPV.SinistraX = um_to_V*Sx_um-15;
            PPV.SinistraY = um_to_V*Sy_um-15;
            
            string ret_msg_a = "Moved Dextra piezo to " + to_string(DextraX) + ", " + to_string(DextraY) + "V (" + to_string(Dx_um) + ", "+ to_string(Dy_um) " microns)";
            string ret_msg_b = "Moved Sinistra piezo to " + to_string(SinistraX) + ", " + to_string(SinistraY) + "V (" + to_string(Sx_um) + ", "+ to_string(Sy_um) " microns)";
            
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
        //double conversion = px_to_um*um_to_V
        double conversion = 1.3134517766497462;
        
        PPV.DextraX += conversion*Dpos.x;
        PPV.DextraY += conversion*Dpos.y;
        PPV.SinistraX += conversion*Spos.x;
        PPV.SinistraY += conversion*Spos.y;
        
        sendPiezoVals(PPV);
        
        return 0;
    }

    piezoStatus moveSciPiezo(double img_px){

        piezoStatus ps;

        if (img_px < XX && img_px >= 0){
            //double img_px_to_img_um = 6.5;
            //double img_um_to_piezo_um = 0.8333333333;
            //double um_to_V = 6.428801028608165;
            //double conversion = img_px_to_img_um*img_um_to_piezo_um*um_to_V;
            double im_px_to_piezo_um = 5.41666666;
            double conversion = 34.82267223815494;
            
            PPV.science = img_px*conversion-15;
            
            string ret_msg = "Moved science piezo to " + to_string(PPV.science) + "V (moved " + to_string(img_px*im_px_to_piezo_um) + " microns)";
            
            sendPiezoVals(PPV);

            ps.um = img_px*im_px_to_piezo_um;
            ps.voltage = img_px*conversion-15;
            ps.msg = ret_msg;
        
        } else {
            ps.um = 0.0;
            ps.voltage = 0.0;
            ps.msg = "Piezo value out of range";
        }
        return ps;
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
        
        int32_t SDC_step_count = teensy_port.current_step;
        SDC_pos = SDC_step_count*0.02;

        cout << "PC Voltage (mV): " << PS.PC_V << endl;
        cout << "PC Current (mA): " << PS.PC_A << endl;
        cout << "Motor Voltage (mV): " << PS.motor_V << endl;
        cout << "Motor Current (mA): " << PS.motor_A << endl;
        cout << "SDC position (um): " << SDC_pos << endl;
    }

    uint8_t roundPWM(double voltage){
        uint8_t ret_val;
        double V_to_PWM = 2.217391304347826;
        long temp = lround(V_to_PWM*(voltage+15));
        ret_val = (uint8_t) temp;
        return temp
    }

    void sendPiezoVals(piezoPWMvals ppv) {
        teensy_port.piezo_duties[0] = roundPWM(ppv.DextraX);
        teensy_port.piezo_duties[1] = roundPWM(ppv.DextraX);
        teensy_port.piezo_duties[2] = roundPWM(ppv.SinistraX);
        teensy_port.piezo_duties[3] = roundPWM(ppv.SinistraY);
        teensy_port.piezo_duties[4] = roundPWM(ppv.science);
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
                     {"SDC_step_count", s.sdc_pos},
                     {"message",s.msg}};
        }

        static void from_json(const json& j, status& p) {
            j.at("PC_current").get_to(s.ps.PC_A);
            j.at("PC_voltage").get_to(s.ps.PC_V);
            j.at("Motor_current").get_to(s.ps.motor_A);
            j.at("Motor_voltage").get_to(s.ps.motor_V);
            j.at("Dextra X").get_to(s.ppv.DextraX);
            j.at("Dextra Y").get_to(s.ppv.DextraY);
            j.at("Sinistra X").get_to(s.ppv.SinistraX);
            j.at("Sinistra Y").get_to(s.ppv.SinistraY);
            j.at("Science").get_to(s.ppv.science);
            j.at("SDC_step_count").get_to(s.sdc_pos);
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
		.def("moveTipTiltPiezos", &ChiefAuxServer::moveTipTiltPiezos, "Set the tip/tilt piezos")
		.def("moveAllTipTiltPiezos", &ChiefAuxServer::moveAllTipTiltPiezos, "Set all the tip/tilt piezos")
		.def("receiveRelativeTipTiltPos", &ChiefAuxServer::receiveRelativeTipTiltPos, "Receive positions to move tip tilt piezos")
		.def("moveSciPiezo", &ChiefAuxServer::moveSciPiezo, "Set the science piezos");
}
