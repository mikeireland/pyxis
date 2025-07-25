/*
These functions below have the following roles, described here as there is no header
file

1) Utility functions:

void resonance(RobotDriver *driver) : Drive the robot in sinusoidal resonances (not used)
void ramp(RobotDriver *driver) : Engineering function (not used)

void unambig(RobotDriver *driver) : An update of the "resonance" function.
void translate(RobotDriver *driver) : Core moving of robot
double saturation(double val) : filter
void track(RobotDriver *driver) :

2) The main robot loop, which is a std::thread

robot_loop

Includes the main state machine based on GLOBAL_SERVER_STATUS

3) The watchdog thread, which checks the robot loop is alive and restarts it if not

4) The class definition of struct RobotControlServer (why a struct and not a class?)

5) The COMMANDER_REGISTER(m) defines the commands that can be sent to the robot controller


*/
#include <iostream>
#include <thread>
#include <string>
#include <pthread.h>
#include <commander/commander.h>
#include <time.h>
#include <vector>
#include "Globals.h"

namespace co = commander;
using namespace std;

sched_param sch_params;

string g_filename = "state_file.csv";
Status g_status = {0.0, 0.0}; // Current roll and pitch angles

std::thread robot_controller_thread;
std::thread watchdog_thread;

// For the watchdog thread
int alive_counter = 1;
int last_alive_counter = 0;

// A convenience local global vor zero velocities;
Velocities g_zero_vel = {
    0.0, // velocity
    0.0, // x
    0.0, // y
    0.0, // z
    0.0, // roll
    0.0, // yaw
    0.0, // pitch
    0.0  // el
};
Velocities g_vel = g_zero_vel;

// Heading gain, plus gians and integral terms for yaw ane elevation control
double h_gain = 0.0;
double g_ygain = 0.0;
double g_egain = 0.0;
double g_yint = 0.0;
double g_eint = 0.0;

double g_esum = 0.0;
double g_ysum = 0.0;

double g_az = 0.0;
double g_alt = 60.0;
double g_posang = 0.0;
double g_az_off = 0.0;
double g_alt_off = 0.0;

struct LEDs {
    double LED1_x; 
    double LED1_y;
    double LED2_x;
    double LED2_y;
};

// A heading is a coarse angle in degrees, which is used to control the robot's direction
double g_heading = 0.0;

// This is the watchdog thread that checks if the robot loop is alive (i.e. hasn't hung) and
// restarts it if it has. 
void watchdog() {
	//start up robot thread procedure
	robot_controller_thread = std::thread(robot_loop);
    //sch_params.sched_priority = 90;
    pthread_setschedparam(robot_controller_thread.native_handle(), SCHED_RR, &sch_params);
	cout << "started\n";
	usleep(1000000);
		// inner loop, while not disconnecting, check robot thread active
		// if not active, kill, restart
		// sleep 100ms
	while(GLOBAL_SERVER_STATUS!=ROBOT_DISCONNECT) {
		if (alive_counter==last_alive_counter) {
			cout << "killing\n";
			pthread_cancel(robot_controller_thread.native_handle());
			robot_controller_thread.join();
			robot_controller_thread = std::thread(robot_loop);
			pthread_setschedparam(robot_controller_thread.native_handle(), SCHED_RR, &sch_params);
		}
		last_alive_counter = alive_counter;
		usleep(100000);

	}
	usleep(100000);

	// send disconnect request
	// wait
	// if not disconnected, kill
	//join
	pthread_cancel(robot_controller_thread.native_handle());
	robot_controller_thread.join();
}

/*
This is the main singleton class, which is for some reason a struct (?)
*/
struct RobotControlServer {

    RobotControlServer(){
        fmt::print("RobotControlServer\n");
        
    }

    ~RobotControlServer(){
        fmt::print("~RobotControlServer\n");
    }

	/*
	Start the robot loop in the 
	*/
	//TO CHANGE
    int start_robot_loop() {
        GLOBAL_SERVER_STATUS = ROBOT_IDLE;
        GLOBAL_STATUS_CHANGED = true;
        watchdog_thread = std::thread(watchdog);
        //sch_params.sched_priority = 90;
        pthread_setschedparam(watchdog_thread.native_handle(), SCHED_RR, &sch_params);
        return 0;
    }


    int stop_robot_loop() {
        // Set all the velocities to zero
        g_vel = g_zero_vel;
        g_ygain = 0;
        g_egain=0;
        h_gain = 0;
        GLOBAL_SERVER_STATUS = ROBOT_TRANSLATE;
        GLOBAL_STATUS_CHANGED = true;
        return 0;
    }


    void translate_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val, double el_val) {
        g_vel.velocity = vel;
        g_vel.x = x_val;
        g_vel.y = y_val;
        g_vel.z = z_val;
        g_vel.roll = roll_val;
        g_vel.yaw = yaw_val;
        g_vel.pitch = pitch_val;
        g_vel.el = el_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_TRANSLATE;
    }

    void resonance_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val) {
        g_vel.velocity = vel;
        g_vel.x = x_val;
        g_vel.y = y_val;
        g_vel.z = z_val;
        g_vel.roll = roll_val;
        g_vel.yaw = yaw_val;
        g_vel.pitch = pitch_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_RESONANCE;
    }
    
    void change_file(string file) {
        g_filename = file;
    }

    void receive_ST_angles(double azimuth, double altitude, double pos_angle) {
    	// Receives the star-tracker angles in arcseconds and stores them in 
    	// global variables az, alt and pos
	    g_az = 206265.0*azimuth;
	    g_alt = 206265.0*altitude;
	    g_posang = 206265.0*pos_angle;
    }

    void set_gains(double y, double e, double yi, double ei) {
    	// Set the gains for tracking altitude (callled "e" or elevation)
    	// and azimuth (called "y" or yaw) 
        // Also set the sums to zero.
	    g_ygain = y;
	    g_egain = e;
		g_yint = yi;
		g_eint = ei;
        g_esum = 0.0;
        g_ysum = 0.0;
    }
    
    void set_heading(double h, double gain) {
        g_heading = 60*h; //!!! Why 60?
        h_gain = gain;
    }

    void track_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val, double el_val) {
        g_vel.velocity = vel;
        g_vel.velocity = vel;
        g_vel.velocity = vel;
        g_vel.x = x_val;
        g_vel.y = y_val;
        g_vel.z = z_val;
        g_vel.roll = roll_val;
        g_vel.yaw = yaw_val;
        g_vel.pitch = pitch_val;
        g_vel.el = el_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_TRACK;
    }

	// Set everything to zero and stop threads.
    int disconnect() {
        g_vel = g_zero_vel;
        g_ygain = 0;
	    g_egain = 0;
        GLOBAL_SERVER_STATUS = ROBOT_DISCONNECT;
        GLOBAL_STATUS_CHANGED = true;
        watchdog_thread.join();
        return 0;
    }

	int offset_targets(double azimuth, double altitude, double rollval, double pitchval) {
		g_az_off += azimuth;
		g_alt_off += altitude;
		g_roll_target += rollval;
		g_pitch_target += pitchval;
		return 0;
	}

	// Receive the LED positions from the coarse metrology, which should be used to offset the 
	// y and z postions of the robot (!!! Not implemented yet !!!)
	int receive_LED(LEDs measured) {
		cout << measured.LED1_x << '\n';
		cout << measured.LED1_y << '\n';
		cout << measured.LED2_x << '\n';
		cout << measured.LED2_y << '\n';
		return 0;
	}

    //Added by Qianhui: receive the misalignment error from the coarse metrology and move correctly
    int receive_AlignmentError(double dlt_p_x, double dlt_p_y) {
        double y_offset = dlt_p_y;
        double z_offset = dlt_p_x;
        double y_mov = 0;
        double z_mov = 0;

        // make robot move to reduce the offset
        if (y_offset < 1e-6 && y_offset > -1e-6) {
            y_mov = 0; // Y axis is aligned, no movement needed
        }
        else{
            y_mov = -y_offset; ;
            if (y_mov > 7.0) y_mov = 7.0;
            if (y_mov < -7.0) y_mov = -7.0;//cap the velocity to 10mm/s
        }

        // If the z_offset is zero, no movement is needed, otherwise move in the z direction
        if (z_offset < 1e-6 && z_offset > -1e-6) {
            z_mov = 0; // Z axis is aligned, no movement needed
        }
        else{
            z_mov = -z_offset;
            if (z_mov > 7.0) z_mov = 7.0;
            if (z_mov < -7.0) z_mov = -7.0; //cap the velocity to 10cm/s
        }

    
        translate_robot(1, 0, y_mov, z_mov, 0.0, 0.0, 0.0, 0.0);
        std::cout << ", moving robot to reduce alignment error: "
                    << "vel(y): " << y_mov << ", vel(z): " << z_mov;
        translate_robot(1,0,0,0,0,0,0,0);
        return 0;
    }



	Status status(){
        // !!! Needs a thead-lock here.
        g_status.loop_counter = loop_counter;
        g_status.loop_status = GLOBAL_SERVER_STATUS;
        return g_status;
	}
};

namespace nlohmann {
    template <>
    struct adl_serializer<LEDs> {
        static void to_json(json& j, const LEDs& L) {
            j = json{{"LED1_x", L.LED1_x},{"LED1_y", L.LED1_y}, 
            {"LED2_x", L.LED2_x},{"LED2_y", L.LED2_y}};
        }

        static void from_json(const json& j, LEDs& L) {
            j.at("LED1_x").get_to(L.LED1_x);
            j.at("LED1_y").get_to(L.LED1_y);
            j.at("LED2_x").get_to(L.LED2_x);
            j.at("LED2_y").get_to(L.LED2_y);
        }
    };

	template <>
	struct adl_serializer<Status> {
        static void to_json(json& j, const Status& L) {
            j = json{{"roll", L.roll},
			{"pitch", L.pitch},
            {"loop_status", L.loop_status},
            {"loop_counter", L.loop_counter}
            };
        }

        static void from_json(const json& j, Status& L) {
			j.at("roll").get_to(L.roll);
			j.at("pitch").get_to(L.pitch);
            j.at("loop_status").get_to(L.loop_status);
            j.at("loop_counter").get_to(L.loop_counter);
        }
    };
}

COMMANDER_REGISTER(m)
{
    // You can register a function or any other callable object as
    // long as the signature is deductible from the type.
    m.instance<RobotControlServer>("RC")
        .def("stop", &RobotControlServer::stop_robot_loop, "A function that stops all motors and stops the robot loop")
        .def("start", &RobotControlServer::start_robot_loop, "A function that starts the robot control loop (in idle)")
        .def("translate", &RobotControlServer::translate_robot, "A function that translates the robot")
        .def("resonance", &RobotControlServer::resonance_robot, "A function that tests robot resonances")
        .def("file", &RobotControlServer::change_file, "Change the name of the log file (unused).")
        .def("receive_ST_angles", &RobotControlServer::receive_ST_angles, "Store the current angle offsets from the Star Tracker.")
        .def("track", &RobotControlServer::track_robot, "Track the star, based on the received angles from the Star Tracker.")
        .def("set_gains", &RobotControlServer::set_gains, "Set the gains for tracking alt/az, i.e. el/yaw")
        .def("set_heading", &RobotControlServer::set_heading, "UNUSED - manual control only")
		.def("receive_LED_positions", &RobotControlServer::receive_LED, "placeholder")
		.def("receive_AlignmentError", &RobotControlServer::receive_AlignmentError, "Receive the Alignment offset from the coarse metrology.")
        .def("update_offsets", &RobotControlServer::offset_targets, "Offset the azimuth and altitude, roll, pitch.")
        .def("disconnect", &RobotControlServer::disconnect, "placeholder")
		.def("status", &RobotControlServer::status, "Get the status of all axes including roll and pitch.");
}
