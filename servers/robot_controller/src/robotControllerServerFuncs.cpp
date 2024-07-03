/*
These functions below have the following roles, described here as there is no header
file
1) Utility functions:

void resonance(RobotDriver *driver) : Drive the robot in sinusoidal resonances
void unambig(RobotDriver *driver) : ??? to delete ???
void translate(RobotDriver *driver) : Core moving of robot
double saturation(double val) : filter
void track(RobotDriver *driver) :
void ramp(RobotDriver *driver) : Engineering function
void stop(RobotDriver *driver) : ??? to delete ???

2) The main robot loop

robot_loop

Includes the main state machine based on GLOBAL_SERVER_STATUS

3) The class definition of struct RobotControlServer (why a struct and not a class?)

4) The COMMANDER_REGISTER(m)

*/
#include <iostream>
#include <thread>
#include <string>
#include <pthread.h>
#include <commander/commander.h>
#include "RobotDriver.h"
#include <time.h>
#include <vector>
#include "Globals.h"

#define ROBOT_IDLE 1
#define ROBOT_TRANSLATE 2
#define ROBOT_UNAMBIG 3
#define ROBOT_TRACK 4
#define ROBOT_DISCONNECT 5


namespace co = commander;
using namespace std;

using namespace Control;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::microseconds;
using std::chrono::seconds;

sched_param sch_params;

auto time_point_start = steady_clock::now();
auto time_point_current = steady_clock::now();
auto now_time = steady_clock::now();
auto last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto last_leveller_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto last_leveller_subtimepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto last_navigator_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto packet_time = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto resonance_time = duration_cast<seconds>(time_point_current-time_point_start).count();
auto last_resonance_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

int GLOBAL_SERVER_STATUS = 1;
int loop_counter = 0;

bool GLOBAL_STATUS_CHANGED = false;

bool closed_loop_enable_flag = true;

string filename = "state_file.csv";

std::thread robot_controller_thread;
std::thread watchdog_thread;

int alive_counter = 1;
int last_alive_counter = 0;

double velocity = 0.0;
double x = 0.0;
double y = 0.0;
double z = 0.0;
double roll = 0.0;
double yaw = 0.0;
double pitch = 0.0;
double el = 0.0;
double g_az = 0.0;
double g_alt = 0.0;
double g_posang = 0.0;
double az_off = 0.0;
double alt_off = 0.0;

double alt_acc = 0.0;
double az_acc = 0.0;

double current_roll = 0.0;
double current_pitch = 0.0;
double roll_error = 0.0;
double pitch_error = 0.0;
double roll_target = 3578.37;
double pitch_target = -6217.54;

struct LEDs {
    double LED1_x; 
    double LED1_y;
    double LED2_x;
    double LED2_y;
};


// !!! This is a heading , in principle based on a magnetometer (which wasn't
// every installed??? 
double heading = 0.0;
double f = 0.5;

/*********** Utility Functions *****************/
void resonance(RobotDriver *driver) {
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	velocity_target.x = 0.001*velocity*x*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.y = 0.001*velocity*y*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.z = 0.001*velocity*z*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.x = 0.001*velocity*roll*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.y = 0.001*velocity*yaw*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.z = 0.001*velocity*pitch*sin(2*3.14159265*f*global_timepoint*0.000001);
	driver->SetNewStabiliserTarget(velocity_target,angle_target);
	driver->stabiliser.enable_flag_ = true;
	if (f > 5) {
		driver->RequestAllStop(); 
		driver->stabiliser.enable_flag_ = false;
	}
	if(global_timepoint-last_stabiliser_timepoint > 1000) {	
		if(driver->stabiliser.enable_flag_) {
			printf("%ld\n",global_timepoint-last_stabiliser_timepoint);
			cout << global_timepoint*0.000001 << '\n';
			driver->StabiliserLoop();

			//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
			//driver->UpdateBFFVelocity(velocity_target);
			driver->UpdateActuatorVelocity(angle_target);
			//driver->WriteLevellerStateToFileAlt(f, velocity_target);
		}
		last_stabiliser_timepoint = global_timepoint;
		if (global_timepoint-last_resonance_timepoint > 5000000 && sin(2*3.14159265*f*global_timepoint*0.000001)<0.1) {
			driver->RequestAllStop();
			sleep(2);
			f = f + 1;
			cout << f << '\n';
			time_point_start = steady_clock::now();
			time_point_current = steady_clock::now();
			last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			last_resonance_timepoint = global_timepoint;
		}

	}
}

void unambig(RobotDriver *driver) {
	// This function makes a periodic change to the robot velocity of amplitude
	// "velocity" or 0.001 * "velocity" in order to test resonances.
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	velocity_target.x = 0.001*velocity*x*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.y = 0.001*velocity*y*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.z = 0.001*velocity*z*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.x = velocity*roll*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.y = velocity*pitch*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.z = 0.001*velocity*yaw*sin(2*3.14159265*f*global_timepoint*0.000001);
	driver->SetNewStabiliserTarget(velocity_target,angle_target);
	driver->stabiliser.enable_flag_ = true;
	
	if(global_timepoint-last_stabiliser_timepoint > 1000) {
		if (f>100) {
			driver->RequestAllStop(); 
			driver->stabiliser.enable_flag_ = false;
			cout << global_timepoint*0.000001 << '\n';
		}	
		if(driver->stabiliser.enable_flag_) {
                       //cout << global_timepoint*0.000001 << '\n';
                       driver->teensy_port.ReadMessage();
			driver->StabiliserLoop();

			//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
			driver->UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, el);
			//driver->UpdateActuatorVelocity(angle_target);
			driver->LogSteps(global_timepoint, filename, f);
			//driver->WriteStabiliserStateToFile();
		}
		if (global_timepoint-last_stabiliser_timepoint > 1500) {
		    last_stabiliser_timepoint = global_timepoint;
		} else {
		    last_stabiliser_timepoint += 1000;
		}
		if (global_timepoint-last_resonance_timepoint > 5000000 && sin(2*3.14159265*f*global_timepoint*0.000001)<0.01) {
			driver->RequestAllStop();
			driver->teensy_port.SendAllRequests();
			sleep(5);
			f = f + 0.5;
			cout << f << '\n';
			time_point_start = steady_clock::now();
			time_point_current = steady_clock::now();
			last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			last_resonance_timepoint = global_timepoint;
		}
		driver->teensy_port.SendAllRequests();

	}
}



void translate(RobotDriver *driver) {
	// Manual translation of forob.
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	driver->teensy_port.ReadMessage();
	driver->PassAccelBytesToLeveller();
	driver->leveller.UpdateTarget();
	double elevation_target = 0.0000048481*el;
	velocity_target.x = 0.001*velocity*x;
	velocity_target.y = 0.001*velocity*y;
	velocity_target.z = 0.001*velocity*z;
	angle_target.x = roll;
	angle_target.y = pitch;
	angle_target.z = 0.0000014302*yaw;
	if (!(loop_counter % 1000)) {
		current_roll = 3600*driver->leveller.roll_estimate_filtered_;
		current_pitch = 3600*driver->leveller.pitch_estimate_filtered_;
		cout << "roll: " << current_roll << '\n';
		cout << "pitch: " << current_pitch << '\n';
	}
	
	//This is where we set a target.
	driver->SetNewStabiliserTarget(velocity_target,angle_target);
	driver->stabiliser.enable_flag_ = true;
	if(1) {	
		if(driver->stabiliser.enable_flag_) {
			//printf("%ld\n",global_timepoint-last_stabiliser_timepoint);
			//if  (global_timepoint < 10000000) {
			//	velocity_target.x = 0;
			//	velocity_target.y = 0;
			//	velocity_target.z = 0;
			//}
			driver->StabiliserLoop();

			//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
			driver->UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, elevation_target);
			//driver->WriteLevellerStateToFileAlt(f, velocity_target);
			driver->teensy_port.SendAllRequests();
		//	driver->LogSteps(global_timepoint, filename, f);

		}
		last_stabiliser_timepoint = global_timepoint;
	}
}

double saturation(double val) {
    if (val > 5000.0) {
        return 5000.0;
    }
    if (val < -5000.0) {
        return -5000.0;
    }
    return val;
}

void track(RobotDriver *driver) {
	// Track the star, based on the g_az and g_alt offsets received previously from
	// the star tracker. This supercedes much of the machinery in the RobotDriver.
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	
	// Here we get the latest data (i.e. accelerometers) from the Teensy.
	driver->teensy_port.ReadMessage();

	// Send the raw data to the leveller object, so that it can compute acclerations.
	driver->PassAccelBytesToLeveller();
	
	// Update our pitch and roll targets (internally in degrees)
	driver->leveller.UpdateTarget();
	
	// The leveller simply estimates the roll and pitch in degrees. We save these
	// in arcseconds. 
	current_roll = 3600*driver->leveller.roll_estimate_filtered_;
	current_pitch = 3600*driver->leveller.pitch_estimate_filtered_;
	roll_error = roll_target - current_roll;
	pitch_error = pitch_target - current_pitch;

	// the constant on the following line is arc-seconds per radian.
	double elevation_target = 0.0000048481*saturation(el + g_egain*(g_alt+alt_off) + g_eint*alt_acc);
	velocity_target.x = 0.001*velocity*x;
	velocity_target.y = 0.001*velocity*y;
	velocity_target.z = 0.001*velocity*z;
	
	// Create velocities for pitch and roll, based on a proportional server and our
	// errors.
	angle_target.x = saturation(roll + roll_gain*roll_error);
	angle_target.y = saturation(pitch + pitch_gain*pitch_error);
	
	// Create a yaw velocity based on the target yaw velocity "yaw" and a PI servo loop
	// based on the g_az.
	angle_target.z = 0.0000014302*saturation(yaw + g_ygain*(g_az + az_off) + g_ygain*az_acc + h_gain*heading);
	
	// This is an integral term (not an acceleration)
	alt_acc += 0.001*(g_alt + alt_off);
	az_acc += 0.001*(g_az + az_off);
				
	// This requests accelerations and step counts (other parts commented out)
	driver->StabiliserLoop();

	//This sends all velocities to the teensy queue of requests.
	driver->UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, elevation_target);
	
	// This sends all request to the Teensy via USB2
	driver->teensy_port.SendAllRequests();

	last_stabiliser_timepoint = global_timepoint;
}

void ramp(RobotDriver *driver) {
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	velocity_target.x = 0.001*velocity*x;
	velocity_target.y = 0.001*velocity*y;
	velocity_target.z = 0.001*velocity*z;
	angle_target.x = velocity*roll;
	angle_target.y = velocity*pitch;
	angle_target.z = 0.001*velocity*yaw;
	driver->SetNewStabiliserTarget(velocity_target,angle_target);
	driver->stabiliser.enable_flag_ = true;
	
	if(1) {	
		if(driver->stabiliser.enable_flag_) {
		    if (global_timepoint*0.000001<2) {
			    double scaler = (double) global_timepoint / 2000000.0;
			    velocity_target.x = 0.001*velocity*x*scaler;
			    velocity_target.y = 0.001*velocity*y*scaler;
			    velocity_target.z = 0.001*velocity*z*scaler;
			    angle_target.x = velocity*roll*scaler;
			    angle_target.y = velocity*pitch*scaler;
			    angle_target.z = 0.001*velocity*yaw*scaler;
			    //
			}
			driver->StabiliserLoop();

			//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
			driver->UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, el);
			//driver->UpdateActuatorVelocity(angle_target);
			driver->LogSteps(global_timepoint, filename, f);
			//driver->WriteStabiliserStateToFile();
		}
		last_stabiliser_timepoint += 1000;

	}
}

/*
This is the main robot loop, that has an interior loop that is run about every 
milli-second, which is forked as a thread from the RobotControlServer's start_robot_loop 
*/
int robot_loop() {
	//Necessary global timing measures
	
    time_point_start = steady_clock::now();
    time_point_current = steady_clock::now();
    last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    last_leveller_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    last_leveller_subtimepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    last_navigator_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    resonance_time = duration_cast<seconds>(time_point_current-time_point_start).count();
    last_resonance_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

	RobotDriver* driver = new RobotDriver();
	closed_loop_enable_flag = true;
	//driver->EngageStabiliser();
	time_point_start = steady_clock::now();
	while(closed_loop_enable_flag) {
		//Boolean to store if we have done anything on this loop and wait a little bit if we haven't
		alive_counter++;
		
		if (GLOBAL_STATUS_CHANGED) {
			time_point_start = steady_clock::now();
			time_point_current = steady_clock::now();
                       last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
                       last_leveller_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
                       last_leveller_subtimepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			//last_resonance_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			GLOBAL_STATUS_CHANGED = false;
		}
		time_point_current = steady_clock::now();
		global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

		switch(GLOBAL_SERVER_STATUS) {
			case ROBOT_IDLE:
				usleep(10);
				break;
			case ROBOT_TRANSLATE:
				translate(driver);
				break;
			case ROBOT_UNAMBIG:
				//resonance(driver);
				unambig(driver);
				break;
			case ROBOT_TRACK:
				track(driver);
				break;
			case ROBOT_DISCONNECT:
			    cout << "disconnecting\n";
			    translate(driver);
			    closed_loop_enable_flag = false;
			default:
			    usleep(10);
				break;
		}
		now_time = steady_clock::now();
		usleep(duration_cast<microseconds>(time_point_current-now_time).count() + 1000);
		//time_point_current = steady_clock::now();
		loop_counter++;
		
	}

	driver->teensy_port.ClosePort();
	delete driver;
    return 0;
}

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
        velocity = 0;
        x = 0;
        y = 0;
        z = 0;
        roll = 0;
        yaw = 0;
        pitch = 0;
        el = 0;
        g_ygain = 0;
        g_egain=0;
        h_gain = 0;
        GLOBAL_SERVER_STATUS = ROBOT_TRANSLATE;
        GLOBAL_STATUS_CHANGED = true;
        return 0;
    }


    void translate_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val, double el_val) {
        velocity = vel;
        x = x_val;
        y = y_val;
        z = z_val;
        roll = roll_val;
        yaw = yaw_val;
        pitch = pitch_val;
        el = el_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_TRANSLATE;
    }

    void resonance_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val) {
        velocity = vel;
        x = x_val;
        y = y_val;
        z = z_val;
        roll = roll_val;
        yaw = yaw_val;
        pitch = pitch_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_UNAMBIG;
    }
    
    void change_file(string file) {
        filename = file;
    }

	void print_level() {
		cout << current_roll << '\n';
		cout << current_pitch << '\n';
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
	    g_ygain = y;
	    g_egain = e;
		g_yint = yi;
		g_eint = ei;
    }
    
    void set_heading(double h, double gain) {
        heading = 60*h;
        h_gain = gain;
    }

    void track_robot(double vel, double x_val, double y_val, double z_val, double roll_val, double pitch_val, double yaw_val, double el_val) {
        velocity = vel;
        x = x_val;
        y = y_val;
        z = z_val;
        roll = roll_val;
        yaw = yaw_val;
        pitch = pitch_val;
        el = el_val;
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = ROBOT_TRACK;
    }
	// TO CHANGE
    int disconnect() {
        velocity = 0;
        x = 0;
        y = 0;
        z = 0;
        roll = 0;
        yaw = 0;
        pitch = 0;
        el = 0;
        g_ygain = 0;
	    g_egain = 0;
        GLOBAL_SERVER_STATUS = ROBOT_DISCONNECT;
        GLOBAL_STATUS_CHANGED = true;
        watchdog_thread.join();
        return 0;
    }

	int offset_targets(double azimuth, double altitude, double rollval, double pitchval) {
		roll_target += rollval;
		pitch_target += pitchval;
		az_off += azimuth;
		alt_off += altitude;
		return 0;
	}

	int receive_LED(LEDs measured) {
		cout << measured.LED1_x << '\n';
		cout << measured.LED1_y << '\n';
		cout << measured.LED2_x << '\n';
		cout << measured.LED2_y << '\n';
		return 0;
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
		.def("print_level", &RobotControlServer::print_level, "placeholder")
		.def("receive_LED_positions", &RobotControlServer::receive_LED, "placeholder")
		.def("update_offsets", &RobotControlServer::offset_targets, "Offset the roll, pitch, azimuth and altitude.")
        .def("disconnect", &RobotControlServer::disconnect, "placeholder");
}
