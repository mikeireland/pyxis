/*
The robot controller runs a main thread via the "robot_loop" function, which exists here together
with required utility function. 
*/
#include "SerialPort.h"
#include "Globals.h"
#include <iostream>
#include <cmath>
#include <fstream>

constexpr double PI = 3.14159265358979323846;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

int GLOBAL_SERVER_STATUS = 1;
int loop_counter = 0;
bool GLOBAL_STATUS_CHANGED = false;

// A pointer to the teensy port, which is used to communicate with the Teensy
Comms::SerialPort *teensy_port=nullptr;

// Timing. Is all of this needed?
auto time_point_start = steady_clock::now();
auto time_point_current = steady_clock::now();
auto last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
auto last_resonance_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

/*
driver->leveller.UpdateTarget(); 	
driver->SetNewStabiliserTarget(velocity_target,angle_target);
driver->leveller.enable_flag_ = true;
BFFblah
*/

struct Leveller {
	Doubles acc0_latest_measurements_;
	Doubles acc1_latest_measurements_;
	Doubles acc2_latest_measurements_;
	Motors motor_steps_measurement_;
	Motors actuator_steps_measurement_;
};

Leveller leveller;
double pitch_estimate_arr_[10] = {0.0};
double roll_estimate_arr_[10] = {0.0};
double pitch_estimate_filtered_ = 0.0;
double roll_estimate_filtered_ = 0.0;

bool resonance_enable_flag = true; 

void PassAccelBytesToLeveller() {
	leveller.acc0_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer0_in_.x[0],teensy_port->accelerometer0_in_.x[1]);
	leveller.acc0_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer0_in_.y[0],teensy_port->accelerometer0_in_.y[1]);
	leveller.acc0_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port->accelerometer0_in_.z[0],teensy_port->accelerometer0_in_.z[1]);
	leveller.acc1_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer1_in_.x[0],teensy_port->accelerometer1_in_.x[1]);
	leveller.acc1_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer1_in_.y[0],teensy_port->accelerometer1_in_.y[1]);
	leveller.acc1_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port->accelerometer1_in_.z[0],teensy_port->accelerometer1_in_.z[1]);
	leveller.acc2_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer2_in_.x[0],teensy_port->accelerometer2_in_.x[1]);
	leveller.acc2_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port->accelerometer2_in_.y[0],teensy_port->accelerometer2_in_.y[1]);
	leveller.acc2_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port->accelerometer2_in_.z[0],teensy_port->accelerometer2_in_.z[1]);
}

// Update the target for the levelling control loop.
void UpdateTarget() {
	Doubles acc_estimate_;
	// Combine the accelerations.
	acc_estimate_.x = (-1*leveller.acc0_latest_measurements_.x+1*leveller.acc1_latest_measurements_.y
                           +leveller.acc2_latest_measurements_.x)/3.0;

    acc_estimate_.y = (-1*leveller.acc0_latest_measurements_.y-1*leveller.acc1_latest_measurements_.x
                                                             +leveller.acc2_latest_measurements_.y)/3.0;

    //We flip the sign on the z component so that gravity is measured downwards
    acc_estimate_.z = (leveller.acc0_latest_measurements_.z
                       +leveller.acc1_latest_measurements_.z
                       +leveller.acc2_latest_measurements_.z)/3.0;
	
					   // Here we average the last 10 measurements.
	for(int i = 1; i <= 9; i++) {
        pitch_estimate_arr_[i-1] = pitch_estimate_arr_[i];
        roll_estimate_arr_[i-1] = roll_estimate_arr_[i];
    }

    pitch_estimate_arr_[9] =   atan(acc_estimate_.x/sqrt(acc_estimate_.z*acc_estimate_.z+
                                                  acc_estimate_.y*acc_estimate_.y))*180.0/PI;
    roll_estimate_arr_[9] = atan(acc_estimate_.y/acc_estimate_.z)*180/PI;

    pitch_estimate_filtered_ = 0.0;
    roll_estimate_filtered_ = 0.0;

    for(int i = 0; i <= 9; i++) {
        pitch_estimate_filtered_ += pitch_estimate_arr_[i]/10.0;
        roll_estimate_filtered_ += roll_estimate_arr_[i]/10.0;
    }
    //last_actuator_velocity_target_ = actuator_velocity_target_;
	//pitch_estimate_filtered_ = pitch_estimate_filtered_ - pitch_target_;
    //roll_estimate_filtered_ = roll_estimate_filtered_ - roll_target_;
}

//Pull all of the accelerations from the device (42 Bytes of return data)
void RequestAccelerations() {
    teensy_port->Request(Acc0Wr);
    teensy_port->Request(Acc1Wr);
    teensy_port->Request(Acc2Wr);
    //teensy_port.Request(Acc3Wr);
    //teensy_port.Request(Acc4Wr);
    //teensy_port.Request(Acc5Wr);
}

//Pull all of the step counts from the device (35 Bytes of return data)
void RequestStepCounts() {
    teensy_port->Request(Step0Wr);
    teensy_port->Request(Step1Wr);
    teensy_port->Request(Step2Wr);
    teensy_port->Request(Step3Wr);
    teensy_port->Request(Step4Wr);
    teensy_port->Request(Step5Wr);
	teensy_port->Request(Step6Wr);
}

void UpdateBFFVelocityAngle(double x, double y, double z, double r, double p, double s, double e) {
	// Set the motor velocities, based on x,y,z, roll, pitch, yaw.
    Doubles motor_velocity_target_;
    Doubles actuator_velocity_target_;
    motor_velocity_target_.x = -y - s;                                                     //Motor 0
    motor_velocity_target_.y = ( -x * sin(PI / 3) + y * cos(PI / 3)) - s;   //Motor 1
    motor_velocity_target_.z = (x * sin(PI / 3) + y * cos(PI / 3)) - s;   //Motor 2
    actuator_velocity_target_.x = z + (0.00027778)*((1.0/388.0)*r - (1.0/672.0)*p); //Actuator 0
    actuator_velocity_target_.y = z + (1.0/336.0)*(0.00027778)*p;                                  //Actuator 1
    actuator_velocity_target_.z = z + (0.00027778)*(- (1.0/388.0)*r - (1.0/672.0)*p);
    PhysicalDoubleToVelocityBytes(motor_velocity_target_.x,
                                  &teensy_port->motor_velocities_out_.x[0],
                                  &teensy_port->motor_velocities_out_.x[1]);
    PhysicalDoubleToVelocityBytes(motor_velocity_target_.y,
                                  &teensy_port->motor_velocities_out_.y[0],
                                  &teensy_port->motor_velocities_out_.y[1]);
    PhysicalDoubleToVelocityBytes(motor_velocity_target_.z,
                                  &teensy_port->motor_velocities_out_.z[0],
                                  &teensy_port->motor_velocities_out_.z[1]);
    PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.x,
								  &teensy_port->actuator_velocities_out_.x[0],
								  &teensy_port->actuator_velocities_out_.x[1]);
	PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.y,
								  &teensy_port->actuator_velocities_out_.y[0],
								  &teensy_port->actuator_velocities_out_.y[1]);
	PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.z,
								  &teensy_port->actuator_velocities_out_.z[0],
								  &teensy_port->actuator_velocities_out_.z[1]);
	PhysicalDoubleToVelocityBytes(e,
								  &teensy_port->goniometer_velocity_out_[0],
								  &teensy_port->goniometer_velocity_out_[1]);
	teensy_port->Request(SetRaw0);
    teensy_port->Request(SetRaw1);
    teensy_port->Request(SetRaw2);
    teensy_port->Request(SetRaw3);	
	teensy_port->Request(SetRaw4);
	teensy_port->Request(SetRaw5);	
	teensy_port->Request(SetRaw6);	
}

bool stabiliser_file_open_flag_ = false;
void LogSteps(int t_step, std::string filename, double f) {
	std::ofstream output; 
	output.open(filename ,std::ios::out);
	
	if(!stabiliser_file_open_flag_){
		//Create a new file and write the state to it
		output << "time" << ',' << "freq" << ',' << "M0_steps" << ',' << "M1_steps" << ',' << "M2_steps" << ',' 
			<< "LA0_steps" << ',' << "LA1_steps" << ',' << "LA2_steps" << ',' 
			<< "accelerometer0_x" << ',' << "accelerometer0_y" << ',' << "accelerometer0_z" << ','
			   << "accelerometer1_x" << ',' << "accelerometer1_y" << ',' << "accelerometer1_z" << ','
			   << "accelerometer2_x" << ',' << "accelerometer2_y" << ',' << "accelerometer2_z" 
			   << '\n';
		//Set the flag to say that the leveller state file is already open for this run
		stabiliser_file_open_flag_ = true;
	}
	output << t_step << ',' << f << ',' << leveller.motor_steps_measurement_.motor_0 << ',' << leveller.motor_steps_measurement_.motor_1 << ','  << leveller.motor_steps_measurement_.motor_2 << ',' 
			   << leveller.actuator_steps_measurement_.motor_0 << ',' << leveller.actuator_steps_measurement_.motor_1 << ',' << leveller.actuator_steps_measurement_.motor_2 << ',' 
			   << leveller.acc0_latest_measurements_.x << ',' << leveller.acc0_latest_measurements_.y << ',' << leveller.acc0_latest_measurements_.z << ','
			   << leveller.acc1_latest_measurements_.x << ',' << leveller.acc1_latest_measurements_.y << ',' << leveller.acc1_latest_measurements_.z << ','
		 	   << leveller.acc2_latest_measurements_.x << ',' << leveller.acc2_latest_measurements_.y << ',' << leveller.acc2_latest_measurements_.z 
			   <<'\n';
	output.close();
}

// Manual translation of robot, for coarse positioning.
void translate() {
	Doubles velocity_target;
	Doubles angle_target;
	teensy_port->ReadMessage();
	PassAccelBytesToLeveller(); 
	UpdateTarget(); 	
	double elevation_target = 0.0000048481*g_vel.el;
	velocity_target.x = 0.001*g_vel.velocity*g_vel.x;
	velocity_target.y = 0.001*g_vel.velocity*g_vel.y;
	velocity_target.z = 0.001*g_vel.velocity*g_vel.z;
	angle_target.x = g_vel.roll;
	angle_target.y = g_vel.pitch;
	angle_target.z = 0.0000014302*g_vel.yaw;
	// Update the current roll every 1000 cycles (why not more often?)
	if (!(loop_counter % 1000)) {
		g_status.roll = 3600*roll_estimate_filtered_;
		g_status.pitch = 3600*pitch_estimate_filtered_;
	}
	
	RequestAccelerations();
	RequestStepCounts();

	//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
	UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, elevation_target);
	
	teensy_port->SendAllRequests();
	last_stabiliser_timepoint = global_timepoint;
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

void track() {
	// Track the star, based on the g_az and g_alt offsets received previously from
	// the star tracker. This supercedes much of the machinery in the RobotDriver.
	Doubles velocity_target;
	Doubles angle_target;
	double roll_error, pitch_error;
	
	// Here we get the latest data (i.e. accelerometers) from the Teensy.
	teensy_port->ReadMessage();

	// Send the raw data to the leveller object, so that it can compute acclerations.
	PassAccelBytesToLeveller();
	
	// Update our pitch and roll targets (internally in degrees)
	UpdateTarget();
	
	// The leveller simply estimates the roll and pitch in degrees. We save these
	// in arcseconds. 
	g_status.roll = 3600*roll_estimate_filtered_;
	g_status.pitch = 3600*pitch_estimate_filtered_;
	roll_error = g_roll_target - g_status.roll;
	pitch_error = g_pitch_target - g_status.pitch;

	// the constant on the following line is arc-seconds per radian.
	double elevation_target = 0.0000048481*saturation(g_vel.el + g_egain*(g_alt+g_alt_off) + g_eint*g_esum);
	velocity_target.x = 0.001*g_vel.velocity*g_vel.x;
	velocity_target.y = 0.001*g_vel.velocity*g_vel.y;
	velocity_target.z = 0.001*g_vel.velocity*g_vel.z;
	
	// Create velocities for pitch and roll, based on a proportional server and our
	// errors.
	angle_target.x = saturation(g_vel.roll + g_roll_gain*roll_error);
	angle_target.y = saturation(g_vel.pitch + g_pitch_gain*pitch_error);
	
	// Create a yaw velocity based on the target yaw velocity "yaw" and a PI servo loop
	// based on the g_az.
	angle_target.z = 0.0000014302*saturation(g_vel.yaw + g_ygain*(g_az + g_az_off) + g_yint*g_ysum + h_gain*g_heading);
	
	// This is an integral term, i.e. a sum.
	g_esum += 0.001*(g_alt + g_alt_off);
	g_ysum += 0.001*(g_az + g_az_off);
				
	// This requests accelerations and step counts (other parts commented out)
	RequestAccelerations();
	RequestStepCounts();

	//This sends all velocities to the teensy queue of requests.
	UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, elevation_target);
	
	// This sends all request to the Teensy via USB2
	teensy_port->SendAllRequests();

	last_stabiliser_timepoint = global_timepoint;
}

// This function makes a periodic change to the robot velocity of amplitude
// "velocity" or 0.001 * "velocity" in order to test resonances. It is an update
// to the original resonance function, which was not working correctly (why?)

void unambig() {	
	double f = 0.5; // Frequency in Hz. This has to be a global and an input!!!
	Doubles velocity_target;
	Doubles angle_target;
	velocity_target.x = 0.001*g_vel.velocity*g_vel.x*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.y = 0.001*g_vel.velocity*g_vel.y*sin(2*3.14159265*f*global_timepoint*0.000001);
	velocity_target.z = 0.001*g_vel.velocity*g_vel.z*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.x = 0.001*g_vel.roll*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.y = 0.001*g_vel.pitch*sin(2*3.14159265*f*global_timepoint*0.000001);
	angle_target.z = 0.001*g_vel.yaw*sin(2*3.14159265*f*global_timepoint*0.000001);
	
	if(global_timepoint-last_stabiliser_timepoint > 1000) {
		if (f>100) {
			teensy_port->Request(STOP);
			g_vel = g_zero_vel;
			resonance_enable_flag = false;
			std::cout << global_timepoint*0.000001 << '\n';
		}	
		if(resonance_enable_flag) {
            teensy_port->ReadMessage();	
			RequestAccelerations();
			RequestStepCounts();
			//As a stress on the messaging, we update the velocity to the same value each time (this is a more realistic version of the system)
			UpdateBFFVelocityAngle(velocity_target.x, velocity_target.y, velocity_target.z, angle_target.x, angle_target.y, angle_target.z, g_vel.el);
			LogSteps(global_timepoint, g_filename, f);
		}
		if (global_timepoint-last_stabiliser_timepoint > 1500) {
		    last_stabiliser_timepoint = global_timepoint;
		} else {
		    last_stabiliser_timepoint += 1000;
		}
		if (global_timepoint-last_resonance_timepoint > 5000000 && sin(2*3.14159265*f*global_timepoint*0.000001)<0.01) {
			teensy_port->Request(STOP);
			g_vel = g_zero_vel;
			teensy_port->SendAllRequests();
			sleep(5);
			f = f + 0.5;
			std::cout << f << '\n';
			time_point_start = steady_clock::now();
			time_point_current = steady_clock::now();
			last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
			last_resonance_timepoint = global_timepoint;
		}
		teensy_port->SendAllRequests();

	}
}

/*
This is the main robot loop, that has an interior loop that is run about every 
milli-second, which is forked as a thread from the RobotControlServer's start_robot_loop 
*/
int robot_loop() {
    // Initialise the communication port to the Teensy
    teensy_port = new Comms::SerialPort(128);

	// Reset the loop counter
	loop_counter = 0;
	
	//Necessary global timing measures (why needed?)
    time_point_start = steady_clock::now();
    time_point_current = steady_clock::now();
    global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
    last_resonance_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

	bool closed_loop_enable_flag = true;
	time_point_start = steady_clock::now();
	while(closed_loop_enable_flag) {
		//Boolean to store if we have done anything on this loop and wait a little bit if we haven't
		alive_counter++;
		
		if (GLOBAL_STATUS_CHANGED) {
			time_point_start = steady_clock::now();
			time_point_current = steady_clock::now();
            last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
 			resonance_enable_flag = true;
			GLOBAL_STATUS_CHANGED = false;
		}
		time_point_current = steady_clock::now();
		global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

		switch(GLOBAL_SERVER_STATUS) {
			case ROBOT_IDLE:
				usleep(10);
				break;
			case ROBOT_TRANSLATE:
				translate();
				break;
			case ROBOT_RESONANCE:
				unambig();
				break;
			case ROBOT_TRACK:
				track();
				break;
			case ROBOT_DISCONNECT:
			    std::cout << "disconnecting\n";
			    translate();
			    closed_loop_enable_flag = false;
			default:
			    usleep(10);
				break;
		}
		usleep(duration_cast<microseconds>(time_point_current-steady_clock::now()).count() + 1000);
		loop_counter++;
		
	}
	teensy_port->ClosePort();
    delete teensy_port;
    return 0;
}