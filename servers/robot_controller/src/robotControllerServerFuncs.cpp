#include <iostream>
#include <thread>
#include <string>
#include <pthread.h>
#include <commander/commander.h>
#include "RobotDriver.h"
#include <time.h>
#include <vector>

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

bool GLOBAL_STATUS_CHANGED = false;

bool closed_loop_enable_flag = true;

string filename = "state_file.csv";

std::thread robot_controller_thread;

double velocity = 0.0;
double x = 0.0;
double y = 0.0;
double z = 0.0;
double roll = 0.0;
double yaw = 0.0;
double pitch = 0.0;
double el = 0.0;
double az = 0.0;
double alt = 0.0;
double pos = 0.0;

double alt_acc = 0.0;
double az_acc = 0.0;

double current_roll = 0.0;
double current_pitch = 0.0;
double roll_error = 0.0;
double pitch_error = 0.0;
double roll_target = 0.0;
double pitch_target = 0.0;
double roll_gain = 0.0000001;
double pitch_gain = 0.0000001;


double heading = 0.0;
double h_gain = 0.0;
double ygain = 0.0;
double egain = 0.0;

double yint = 0.0;
double eint = 0.0;

double f = 0.5;

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
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	double elevation_target = 0.0000048481*el;
	velocity_target.x = 0.001*velocity*x;
	velocity_target.y = 0.001*velocity*y;
	velocity_target.z = 0.001*velocity*z;
	angle_target.x = roll;
	angle_target.y = pitch;
	angle_target.z = 0.0000014302*yaw;
	
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
			driver->teensy_port.ReadMessage();
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
	Servo::Doubles velocity_target;
	Servo::Doubles angle_target;
	driver->teensy_port.ReadMessage();
	driver->leveller.UpdateTarget();
	current_roll = 3600*driver->leveller.roll_estimate_filtered_;
	current_pitch = 3600*driver->leveller.pitch_estimate_filtered_;
	roll_error = roll_target - current_roll;
	pitch_error = pitch_target - current_pitch;
	double elevation_target = 0.0000048481*saturation(el + egain*alt + eint*alt_acc);
	velocity_target.x = 0.001*velocity*x;
	velocity_target.y = 0.001*velocity*y;
	velocity_target.z = 0.001*velocity*z;
	angle_target.x = saturation(roll+roll_gain*roll_error);
	angle_target.y = saturation(pitch + pitch_gain*pitch_error);
	angle_target.z = 0.0000014302*saturation(yaw + ygain*az + yint*az_acc + h_gain*heading);

	alt_acc += 0.001*alt;
	az_acc += 0.001*az;
	
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
			    //cout << scaler << '\n';
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

void level(RobotDriver *driver) {
/*
    if (driver->leveller.enable_flag_) {
        if (global_timepoint-last_leveller_subtimepoint > 1000) {
            driver->teensy_port.ReadMessage();
            driver->RequestAccelerations();
	        driver->PassAccelBytesToLeveller();
	        driver->leveller.UpdateTarget();
	        driver->UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, el);
        }
    }
    else {
        driver->SetNewTargetAngle(0.0, 0.0);
        driver->EngageLeveller();
    }
*/
}

void stop(RobotDriver *driver) {
	for(int i = 0; i < 100; i++) {
		driver->RequestAllStop();
		driver->teensy_port.PacketManager();
		usleep(1000);
	}
}

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
	driver->EngageStabiliser();
	time_point_start = steady_clock::now();
	while(closed_loop_enable_flag) {
		//Boolean to store if we have done anything on this loop and wait a little bit if we haven't
		
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
			case 1:
				usleep(10);
				break;
			case 2:
				translate(driver);
				break;
			case 3:
				//resonance(driver);
				unambig(driver);
				break;
			case 4:
				translate(driver);
				break;
			case 5:
				level(driver);
				break;
			case 6:
				track(driver);
				break;
			case 7:
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

		
	}

	driver->teensy_port.ClosePort();
	delete driver;
    return 0;
}

struct RobotControlServer {

    RobotControlServer(){
        fmt::print("RobotControlServer\n");
        
    }

    ~RobotControlServer(){
        fmt::print("~RobotControlServer\n");
    }

    int start_robot_loop() {
        GLOBAL_SERVER_STATUS = 1;
        GLOBAL_STATUS_CHANGED = true;
        cout << "fine";
        robot_controller_thread = std::thread(robot_loop);
        //sch_params.sched_priority = 90;
        pthread_setschedparam(robot_controller_thread.native_handle(), SCHED_RR, &sch_params);
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
        ygain = 0;
        egain=0;
        h_gain = 0;
        GLOBAL_SERVER_STATUS = 4;
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
        GLOBAL_SERVER_STATUS = 2;
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
        GLOBAL_SERVER_STATUS = 3;
    }

    void level_robot() {
        GLOBAL_STATUS_CHANGED = true;
        GLOBAL_SERVER_STATUS = 5;
    }

    void change_file(string file) {
        filename = file;
    }

	void print_level() {
		cout << current_roll << '\n';
		cout << current_pitch << '\n';
	}

    void receive_ST_angles(double azimuth, double altitude, double pos_angle) {
	    az = 206265.0*azimuth;
	    alt = 206265.0*altitude;
	    pos = 206265.0*pos_angle;
    }

    void set_gains(double y, double e) {
	    ygain = y;
	    egain = e;
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
        GLOBAL_SERVER_STATUS = 6;
    }

    int disconnect() {
        velocity = 0;
        x = 0;
        y = 0;
        z = 0;
        roll = 0;
        yaw = 0;
        pitch = 0;
        el = 0;
        ygain = 0;
	    egain = 0;
        GLOBAL_SERVER_STATUS = 7;
        GLOBAL_STATUS_CHANGED = true;
        robot_controller_thread.join();
        return 0;
    }
};

COMMANDER_REGISTER(m)
{
    // You can register a function or any other callable object as
    // long as the signature is deductible from the type.
    m.instance<RobotControlServer>("RC")
        .def("stop", &RobotControlServer::stop_robot_loop, "A function that stops all motors and stops the robot loop")
        .def("start", &RobotControlServer::start_robot_loop, "A function that starts the robot control loop (in idle)")
        .def("translate", &RobotControlServer::translate_robot, "A function that translates robot")
        .def("resonance", &RobotControlServer::resonance_robot, "A function that translates robot")
        .def("level", &RobotControlServer::level_robot, "placeholder")
        .def("file", &RobotControlServer::change_file, "placeholder")
        .def("receive_ST_angles", &RobotControlServer::receive_ST_angles, "placeholder")
        .def("track", &RobotControlServer::track_robot, "placeholder")
        .def("set_gains", &RobotControlServer::set_gains, "placeholder")
        .def("set_heading", &RobotControlServer::set_heading, "placeholder")
		.def("print_level", &RobotControlServer::print_level, "placeholder")
        .def("disconnect", &RobotControlServer::disconnect, "placeholder");
}
