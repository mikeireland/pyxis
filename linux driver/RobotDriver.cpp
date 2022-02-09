//Controller.cpp
#include "RobotDriver.h"

#define PI 3.14159265

using namespace Control;

/*
MOTION IN THE PLANE 
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Ramps the velocity of the deputy up linear from zero to the velocity target from 
//all stop over time seconds
void RobotDriver::LinearLateralRamp(Servo::Doubles velocity_target, double time) {
    Servo::Doubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.x = velocity_scale*velocity_target.x; 
        velocity_target_wrapper.y = velocity_scale*velocity_target.y; 
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.PacketManager();
        usleep(1000);     
    }
}

//Ramps the yaw velocity of the deputy up linear from zero to the velocity target from 
//all stop over time seconds
void RobotDriver::LinearYawRamp(double yaw_rate_target, double time) {
    Servo::Doubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.z = velocity_scale*yaw_rate_target;  
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.PacketManager();
        usleep(1000);     
    }
}

//Requests an update of the Body Fixed Frame velocity of the robot
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::UpdateBFFVelocity(Servo::Doubles velocities) {
    //Write the new velocity as the velocity target
    BFF_velocity_target_ = velocities;

    //We then convert from the target BFF velocitities to the 
    //real velocity of each of the motors 
    motor_velocities_target_.x = -BFF_velocity_target_.y - BFF_velocity_target_.z;
    motor_velocities_target_.y = ( - BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    motor_velocities_target_.z = (BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;

    //Send that velocity to the port as a byte array
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.x,
                                  &teensy_port.motor_velocities_out_.x[0],
                                  &teensy_port.motor_velocities_out_.x[1]);
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.y,
                                  &teensy_port.motor_velocities_out_.y[0],
                                  &teensy_port.motor_velocities_out_.y[1]);
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.z,
                                  &teensy_port.motor_velocities_out_.z[0],
                                  &teensy_port.motor_velocities_out_.z[1]);

    //Command to device to update to the new velocities
    teensy_port.Request(SetRaw0);
    teensy_port.Request(SetRaw1);
    teensy_port.Request(SetRaw2);
}

void RobotDriver::RequestNewVelocity(Servo::Doubles velocities) {
    //Write the new velocity as the velocity target
    motor_velocities_target_ = velocities;

    //Send that velocity to the port as a byte array
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.x,
                                  &teensy_port.motor_velocities_out_.x[0],
                                  &teensy_port.motor_velocities_out_.x[1]);
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.y,
                                  &teensy_port.motor_velocities_out_.y[0],
                                  &teensy_port.motor_velocities_out_.y[1]);
    PhysicalDoubleToVelocityBytes(motor_velocities_target_.z,
                                  &teensy_port.motor_velocities_out_.z[0],
                                  &teensy_port.motor_velocities_out_.z[1]);

    //Command to device to update to the new velocities
    teensy_port.Request(SetRaw0);
    teensy_port.Request(SetRaw1);
    teensy_port.Request(SetRaw2);
}

/*
ACTUATOR CONTROL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Sets the actuators to raise or lower the robot and some velocity
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::UpdateZVelocity(double z_velocity) {
	//Wrtie the new raise and lower velocity into 
	//all of the outgoing actuator velocity cache 
	//The negative sign corresponds to the fact a positive actuator velocity lowers the robot
    PhysicalDoubleToVelocityBytes(-z_velocity,
								  &teensy_port.actuator_velocities_out_.x[0],
								  &teensy_port.actuator_velocities_out_.x[1]);
	PhysicalDoubleToVelocityBytes(-z_velocity,
								  &teensy_port.actuator_velocities_out_.y[0],
								  &teensy_port.actuator_velocities_out_.y[1]);
	PhysicalDoubleToVelocityBytes(-z_velocity,
								  &teensy_port.actuator_velocities_out_.z[0],
								  &teensy_port.actuator_velocities_out_.z[1]);
	teensy_port.Request(SetRaw3);	
	teensy_port.Request(SetRaw4);
	teensy_port.Request(SetRaw5);	
}

//Sets the actuators to some velocity (this is intended for use in the leveling servo loop and si is designed to pitch and roll)
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::UpdateActuatorVelocity(Servo::Doubles velocity) {
	//Write the new velocity as the velocity target
	//Note that x,y,z corresponds to actuators 0,1,2
    actuator_velocity_target_ = velocity;

	//Wrtie the new actuator velocities into the outgoing actuator velocity cache 
	//The negative sign corresponds to the fact a positive actuator velocity lowers the robot
    PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.x,
								  &teensy_port.actuator_velocities_out_.x[0],
								  &teensy_port.actuator_velocities_out_.x[1]);
	PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.y,
								  &teensy_port.actuator_velocities_out_.y[0],
								  &teensy_port.actuator_velocities_out_.y[1]);
	PhysicalDoubleToVelocityBytes(-actuator_velocity_target_.z,
								  &teensy_port.actuator_velocities_out_.z[0],
								  &teensy_port.actuator_velocities_out_.z[1]);
	teensy_port.Request(SetRaw3);	
	teensy_port.Request(SetRaw4);
	teensy_port.Request(SetRaw5);	
}

//Ramps the actuators from one velocity to another over some time period
void RobotDriver::LinearActuatorRamp(Servo::Doubles velocity_initial,Servo::Doubles velocity_target, double time) {
    Servo::Doubles velocity_target_temp;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
		velocity_target_temp.x = velocity_initial.x+velocity_scale*(velocity_target.x-velocity_initial.x); 
    	velocity_target_temp.y = velocity_initial.y+velocity_scale*(velocity_target.y-velocity_initial.y); 
		velocity_target_temp.z = velocity_initial.z+velocity_scale*(velocity_target.z-velocity_initial.z); 
    	UpdateActuatorVelocity(velocity_target_temp);
    	teensy_port.PacketManager();
    	usleep(1000);     
    }
}

//Lowers the actuators uniformly for much longer than is required for them to hit the end of their throw
//Once they hit the limit switches they will stop lowering (this is handled on the device)
//We set this point as the step count reference
void RobotDriver::LowerToReference() {
	//Lower all actuators at 1mm/s for 60s
	RequestAllStop();
	UpdateZVelocity(-0.001);
	teensy_port.PacketManager();
	sleep(60);
	RequestAllStop();
	RequestResetStepCount();
}


/*
LEVELLER CONTROL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Converts the last incoming accelerometer values into doubles and passes them to the Leveller
void RobotDriver::SetNewTargetAngle(double pitch, double roll) {
	leveller.pitch_target_ = pitch;
	leveller.roll_target_ = roll;
}

void RobotDriver::PassAccelBytesToLeveller() {
	leveller.acc0_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]);
	leveller.acc0_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]);
	leveller.acc0_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]);
	leveller.acc1_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]);
	leveller.acc1_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]);
	leveller.acc1_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]);
	leveller.acc2_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]);
	leveller.acc2_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]);
	leveller.acc2_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]);
}

void RobotDriver::PassActuatorStepsToLeveller() {
	leveller.current_step_count_.motor_0 = BytesToInt(teensy_port.step_count3_in_[0],teensy_port.step_count3_in_[1],teensy_port.step_count3_in_[2],teensy_port.step_count3_in_[3]);
	leveller.current_step_count_.motor_1 = BytesToInt(teensy_port.step_count4_in_[0],teensy_port.step_count4_in_[1],teensy_port.step_count4_in_[2],teensy_port.step_count4_in_[3]);
	leveller.current_step_count_.motor_2 = BytesToInt(teensy_port.step_count5_in_[0],teensy_port.step_count5_in_[1],teensy_port.step_count5_in_[2],teensy_port.step_count5_in_[3]);
}

void RobotDriver::EngageLeveller() {
	
	LowerToReference();
	
	//Move the platform up from its reference by 20mm
	UpdateZVelocity(0.001);
	teensy_port.PacketManager();
	sleep(20);
	RequestAllStop();

	if(short_level_flag2_) {
		leveller.pitch_target_cache_ = leveller.pitch_target_;
		leveller.roll_target_cache_ = leveller.roll_target_;
		leveller.pitch_target_ = 0.0;
		leveller.roll_target_ = 0.0;
	}
	
	leveller.enable_flag_ = true;
}

void RobotDriver::LevellerLoop() {
	RequestAccelerations();
	teensy_port.PacketManager();

	PassAccelBytesToLeveller();
	leveller.UpdateTarget();
	UpdateActuatorVelocity(leveller.actuator_velocity_target_);
	WriteLevellerStateToFile();
}

void RobotDriver::LevellerSubLoop() {
	RequestAccelerations();
	teensy_port.PacketManager();
	PassAccelBytesToLeveller();
	leveller.UpdateTarget();
}


/*
NAVIGATOR CONTROL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Requests an update of the motor velocities of the robot (this is the same as the UpdateBFFVelocity, but allows for BFF to motor velocity
//conversion to be done in the servo controller rather than in message transit.)
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::PassMotorStepsToNavigator() {
	navigator.current_step_count_.motor_0 = BytesToInt(teensy_port.step_count0_in_[0],teensy_port.step_count0_in_[1],teensy_port.step_count0_in_[2],teensy_port.step_count0_in_[3]);
	navigator.current_step_count_.motor_1 = BytesToInt(teensy_port.step_count1_in_[0],teensy_port.step_count1_in_[1],teensy_port.step_count1_in_[2],teensy_port.step_count1_in_[3]);
	navigator.current_step_count_.motor_2 = BytesToInt(teensy_port.step_count2_in_[0],teensy_port.step_count2_in_[1],teensy_port.step_count2_in_[2],teensy_port.step_count2_in_[3]);
}

//Set the Navigator object's target position to a new location
void RobotDriver::SetNewTargetPosition(Servo::Doubles position_target) {
	RequestAllStop();
	RequestResetStepCount();
	navigator.SetNewPosition(position_target);
}

void RobotDriver::EngageNavigator() {
	RequestStepCounts();
	navigator.enable_flag_ = true;
}

void RobotDriver::NavigatorLoop() {	
	PassMotorStepsToNavigator();
	navigator.UpdateTarget();
	RequestNewVelocity(navigator.motor_velocity_target_);
	RequestStepCounts();
}

void RobotDriver::NavigatorTest() {
	Servo::Doubles target_position;
	target_position.x = 0.2;
	target_position.y = 0.2;
	target_position.z = 0;

	SetNewTargetPosition(target_position);
}

/*
STABILISER CONTROL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void RobotDriver::PassAccelBytesToStabiliser() {
	//Note that we flip the z components so that the gravity vector points down
	stabiliser.acc0_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]);
	stabiliser.acc0_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]);
	stabiliser.acc0_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]);
	stabiliser.acc1_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]);
	stabiliser.acc1_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]);
	stabiliser.acc1_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]);
	stabiliser.acc2_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]);
	stabiliser.acc2_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]);
	stabiliser.acc2_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]);
	stabiliser.acc3_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.x[0],teensy_port.accelerometer3_in_.x[1]);
	stabiliser.acc3_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.y[0],teensy_port.accelerometer3_in_.y[1]);
	stabiliser.acc3_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]);
	stabiliser.acc4_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.x[0],teensy_port.accelerometer4_in_.x[1]);
	stabiliser.acc4_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.y[0],teensy_port.accelerometer4_in_.y[1]);
	stabiliser.acc4_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]);
	stabiliser.acc5_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.x[0],teensy_port.accelerometer5_in_.x[1]);
	stabiliser.acc5_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.y[0],teensy_port.accelerometer5_in_.y[1]);
	stabiliser.acc5_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1]);
}

void RobotDriver::PassStepsToStabiliser() {
	stabiliser.motor_steps_measurement_.motor_0 = BytesToInt(teensy_port.step_count0_in_[0],teensy_port.step_count0_in_[1],teensy_port.step_count0_in_[2],teensy_port.step_count0_in_[3]);
	stabiliser.motor_steps_measurement_.motor_1 = BytesToInt(teensy_port.step_count1_in_[0],teensy_port.step_count1_in_[1],teensy_port.step_count1_in_[2],teensy_port.step_count1_in_[3]);
	stabiliser.motor_steps_measurement_.motor_2 = BytesToInt(teensy_port.step_count2_in_[0],teensy_port.step_count2_in_[1],teensy_port.step_count2_in_[2],teensy_port.step_count2_in_[3]);
	stabiliser.actuator_steps_measurement_.motor_0 = BytesToInt(teensy_port.step_count3_in_[0],teensy_port.step_count3_in_[1],teensy_port.step_count3_in_[2],teensy_port.step_count3_in_[3]);
	stabiliser.actuator_steps_measurement_.motor_1 = BytesToInt(teensy_port.step_count4_in_[0],teensy_port.step_count4_in_[1],teensy_port.step_count4_in_[2],teensy_port.step_count4_in_[3]);
	stabiliser.actuator_steps_measurement_.motor_2 = BytesToInt(teensy_port.step_count5_in_[0],teensy_port.step_count5_in_[1],teensy_port.step_count5_in_[2],teensy_port.step_count5_in_[3]);
}

void RobotDriver::StabiliserLoop() {
	PassAccelBytesToStabiliser();
	PassStepsToStabiliser();
	stabiliser.UpdateTarget();
	WriteStabiliserStateToFile();

	RequestAccelerations();
	RequestStepCounts();

	RequestNewVelocity(stabiliser.motor_velocity_target_);
	UpdateActuatorVelocity(stabiliser.actuator_velocity_target_);	
	}

void RobotDriver::StabiliserSetup() {
	//We initially run the leveller for 20 seconds to both set the reference for the actuators and 
	//to determine the ground state accelerations
	RequestResetStepCount();
	teensy_port.PacketManager();

	SetNewTargetAngle(stabiliser.angle_reference_.y*180.0/PI,stabiliser.angle_reference_.x*180.0/PI);
	short_level_flag_ = true;
	short_level_flag2_ = true;
	EngageLeveller();
}

void RobotDriver::EngageStabiliser() {
	//Upon engaging the stabiliser we push the current accelerations
	//to the ground state value
	RequestAllStop();
	sleep(1);

	for(int i = 0; i < 1000; i++) {
		RequestAccelerations();
		RequestStepCounts();
		teensy_port.PacketManager();

		stabiliser.acc0_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]);
		stabiliser.acc0_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]);
		stabiliser.acc0_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]);
		stabiliser.acc1_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]);
		stabiliser.acc1_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]);
		stabiliser.acc1_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]);
		stabiliser.acc2_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]);
		stabiliser.acc2_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]);
		stabiliser.acc2_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]);
		stabiliser.acc3_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.x[0],teensy_port.accelerometer3_in_.x[1]);
		stabiliser.acc3_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.y[0],teensy_port.accelerometer3_in_.y[1]);
		stabiliser.acc3_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]);
		stabiliser.acc4_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.x[0],teensy_port.accelerometer4_in_.x[1]);
		stabiliser.acc4_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.y[0],teensy_port.accelerometer4_in_.y[1]);
		stabiliser.acc4_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]);
		stabiliser.acc5_ground_state_measurement_.x += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.x[0],teensy_port.accelerometer5_in_.x[1]);
		stabiliser.acc5_ground_state_measurement_.y += 0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.y[0],teensy_port.accelerometer5_in_.y[1]);
		stabiliser.acc5_ground_state_measurement_.z += -0.001*AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1]);
		usleep(500);
	}

	printf("acc0x = %f\n",stabiliser.acc0_ground_state_measurement_.x);
	printf("acc0y = %f\n",stabiliser.acc0_ground_state_measurement_.y);
	printf("acc0z = %f\n",stabiliser.acc0_ground_state_measurement_.z);
	printf("acc1x = %f\n",stabiliser.acc1_ground_state_measurement_.x);
	printf("acc1y = %f\n",stabiliser.acc1_ground_state_measurement_.y);
	printf("acc1z = %f\n",stabiliser.acc1_ground_state_measurement_.z);
	printf("acc2x = %f\n",stabiliser.acc2_ground_state_measurement_.x);
	printf("acc2y = %f\n",stabiliser.acc2_ground_state_measurement_.y);
	printf("acc2z = %f\n",stabiliser.acc2_ground_state_measurement_.z);
	printf("acc3x = %f\n",stabiliser.acc3_ground_state_measurement_.x);
	printf("acc3y = %f\n",stabiliser.acc3_ground_state_measurement_.y);
	printf("acc3z = %f\n",stabiliser.acc3_ground_state_measurement_.z);
	printf("acc4x = %f\n",stabiliser.acc4_ground_state_measurement_.x);
	printf("acc4y = %f\n",stabiliser.acc4_ground_state_measurement_.y);
	printf("acc4z = %f\n",stabiliser.acc4_ground_state_measurement_.z);
	printf("acc5x = %f\n",stabiliser.acc5_ground_state_measurement_.x);
	printf("acc5y = %f\n",stabiliser.acc5_ground_state_measurement_.y);
	printf("acc5z = %f\n",stabiliser.acc5_ground_state_measurement_.z);

	sleep(1);

	
	/*
	stabiliser.acc0_ground_state_measurement_.x = 0.0;
	stabiliser.acc0_ground_state_measurement_.y = 0.0;
	stabiliser.acc0_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]);
	stabiliser.acc1_ground_state_measurement_.x = 0.0;
	stabiliser.acc1_ground_state_measurement_.y = 0.0;
	stabiliser.acc1_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]);
	stabiliser.acc2_ground_state_measurement_.x = 0.0;
	stabiliser.acc2_ground_state_measurement_.y = 0.0;
	stabiliser.acc2_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]);
	stabiliser.acc3_ground_state_measurement_.x = 0.0;
	stabiliser.acc3_ground_state_measurement_.y = 0.0;
	stabiliser.acc3_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]);
	stabiliser.acc4_ground_state_measurement_.x = 0.0;
	stabiliser.acc4_ground_state_measurement_.y = 0.0;
	stabiliser.acc4_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]);
	stabiliser.acc5_ground_state_measurement_.x = 0.0;
	stabiliser.acc5_ground_state_measurement_.y = 0.0;
	stabiliser.acc5_ground_state_measurement_.z = -AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1]);
	*/

	//We also reset the height target to the current height
	PassStepsToStabiliser();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
	stabiliser.height_target_ = 0.0;
	stabiliser.time_ = 0;

	//We switch over which controller is enabled
	stabiliser.enable_flag_ = true;
	leveller.enable_flag_ = false;
	short_level_flag_ = false;

	stabiliser.plat_pos_estimate_.p = leveller.pitch_estimate_filtered_*PI/180.0;
	stabiliser.plat_pos_estimate_.r = leveller.roll_estimate_filtered_*PI/180.0;
	stabiliser.opto_pos_estimate_.p = leveller.pitch_estimate_filtered_*PI/180.0;
	stabiliser.opto_pos_estimate_.r = leveller.roll_estimate_filtered_*PI/180.0;



	//We then pull a set of data to fill the Stabiliser's buffers
	RequestAccelerations();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassAccelBytesToStabiliser();
	PassStepsToStabiliser();
	stabiliser.UpdateTarget();
}

void RobotDriver::SetNewStabiliserTarget(Servo::Doubles velocity_target, Servo::Doubles angle_target) {
	stabiliser.BFF_vel_reference_ = velocity_target;
	stabiliser.angle_reference_ = angle_target;
}

void RobotDriver::StabiliserTest() {
	Servo::Doubles temp_velocity;
	temp_velocity.x = 0.0025;
	temp_velocity.y = 0.0;
	temp_velocity.z = 0.0;

	Servo::Doubles temp_angle;
	//Set an angle of 2 degrees in pitch and roll
	temp_angle.x = 0.0;//(PI/180.0)*2.0;
	temp_angle.y = 0.0;//(PI/180.0)*2.0;
	temp_angle.z = 0.0;

	SetNewStabiliserTarget(temp_velocity,temp_angle);
}

/*
GENERAL CONTROL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Pull all of the accelerations from the device (42 Bytes of return data)
void RobotDriver::RequestAccelerations() {
    teensy_port.Request(Acc0Wr);
    teensy_port.Request(Acc1Wr);
    teensy_port.Request(Acc2Wr);
    teensy_port.Request(Acc3Wr);
    teensy_port.Request(Acc4Wr);
    teensy_port.Request(Acc5Wr);
}

//Pull all of the step counts from the device (30 Bytes of return data)
void RobotDriver::RequestStepCounts() {
    teensy_port.Request(Step0Wr);
    teensy_port.Request(Step1Wr);
    teensy_port.Request(Step2Wr);
    teensy_port.Request(Step3Wr);
    teensy_port.Request(Step4Wr);
    teensy_port.Request(Step5Wr);
}

//Set the motor and actuator velocities to zero
void RobotDriver::RequestAllStop() {
    teensy_port.Request(STOP);
    BFF_velocity_target_.x = 0;
    BFF_velocity_target_.y = 0;
    BFF_velocity_target_.z = 0;
    motor_velocities_target_.x = 0;
    motor_velocities_target_.y = 0;
    motor_velocities_target_.z = 0;
	actuator_velocity_target_.x = 0;
    actuator_velocity_target_.y = 0;
    actuator_velocity_target_.z = 0;
    teensy_port.PacketManager();
    usleep(1000);
}

//Command to robot to reset its step count
void RobotDriver::RequestResetStepCount() {
    teensy_port.Request(ResetSteps);
    navigator.current_step_count_.motor_0 = 0;
	navigator.current_step_count_.motor_1 = 0;
	navigator.current_step_count_.motor_2 = 0;
	leveller.current_step_count_.motor_0 = 0;
	leveller.current_step_count_.motor_1 = 0;
	leveller.current_step_count_.motor_2 = 0;
	stabiliser.motor_steps_measurement_.motor_0 = 0;
	stabiliser.motor_steps_measurement_.motor_1 = 0;
	stabiliser.motor_steps_measurement_.motor_2 = 0;
	stabiliser.actuator_steps_measurement_.motor_0 = 0;
	stabiliser.actuator_steps_measurement_.motor_1 = 0;
	stabiliser.actuator_steps_measurement_.motor_2 = 0;
    teensy_port.PacketManager();
    usleep(1000);
}

/*
DEBUGGING AND I/O
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Command the robot to linearly sweep up to its maximum velocity 
//we also occasionally read off the accelerations as a debugging aid
void RobotDriver::LinearSweepTest() {
    Servo::Doubles vel_target_wrapper;

    //Speed up in the x direction and then reverse
    vel_target_wrapper.x = 0.014/1.414;
    vel_target_wrapper.y = 0.014/1.414;
    LinearLateralRamp(vel_target_wrapper,10);
    RequestAccelerations();
    teensy_port.Request(RUNTIME);
    teensy_port.PacketManager();
    PrintRuntime();
    PrintAccelerations();   
    RequestAllStop();   
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 

    vel_target_wrapper.x = -0.014/1.414;
    vel_target_wrapper.y = -0.014/1.414;
    LinearLateralRamp(vel_target_wrapper,10);
    RequestAccelerations();
    teensy_port.Request(RUNTIME);
    teensy_port.PacketManager();
    PrintRuntime();
    PrintAccelerations();  
    RequestAllStop();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 

    LinearYawRamp(0.014,10);
    RequestAccelerations();
    teensy_port.Request(RUNTIME);
    teensy_port.PacketManager();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 

    LinearYawRamp(-0.014,10);
    RequestAccelerations();
    teensy_port.Request(RUNTIME);
    teensy_port.PacketManager();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 
}

void RobotDriver::RaiseAndLowerTest() {
	RequestAllStop();
	
	
	UpdateZVelocity(0.001);
	RequestAccelerations();
	teensy_port.Request(RUNTIME);
	teensy_port.PacketManager();
	usleep(1000);
	PrintRuntime();
	PrintAccelerations();
	sleep(10);
	RequestAllStop();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 
	

	UpdateZVelocity(-0.001);
	RequestAccelerations();
	teensy_port.Request(RUNTIME);
	teensy_port.PacketManager();;
	usleep(1000);
	PrintRuntime();
	PrintAccelerations();
	sleep(10);
	RequestAllStop();
	RequestStepCounts();
	teensy_port.PacketManager();
	PassMotorStepsToNavigator();
	PassActuatorStepsToLeveller();
	PrintStepCounts(); 
}

void RobotDriver::MeasureOrientationMeasurementNoise() {
	for(int i = 0; i < 10000; ++i) {
		//Each millisecond we request the current accelerometer measurements
		RequestAccelerations();
		teensy_port.PacketManager();

		//We pass that data to the leveller so it can convert it into pitch and roll estimates
		PassAccelBytesToLeveller();
		leveller.UpdateTarget();
		//Write the current state of the leveller to a file
		WriteLevellerStateToFile();
		usleep(500);
	}
}

#include <iostream>
#include <fstream>
void RobotDriver::WriteLevellerStateToFile() {
	if(!leveller_file_open_flag_){
		//Create a new file and write the state to it
		std::ofstream output; 
		output.open("leveller_state.csv",std::ios::out);
		if(output.is_open()) {printf("Leveller File opened correctly\n");}
		output << leveller.pitch_estimate_filtered_ << ',' << leveller.roll_estimate_filtered_ << ','
		 	   << leveller.acc0_latest_measurements_.x << ',' << leveller.acc0_latest_measurements_.y << ',' << leveller.acc0_latest_measurements_.z << ','
			   << leveller.acc1_latest_measurements_.x << ',' << leveller.acc1_latest_measurements_.y << ',' << leveller.acc1_latest_measurements_.z << ','
		 	   << leveller.acc2_latest_measurements_.x << ',' << leveller.acc2_latest_measurements_.y << ',' << leveller.acc2_latest_measurements_.z << ','
			   << leveller.acc_estimate_.x << ',' << leveller.acc_estimate_.y << ',' << leveller.acc_estimate_.z << ','
			   << leveller.actuator_velocity_target_.x << ',' << leveller.actuator_velocity_target_.y << ',' << leveller.actuator_velocity_target_.z 
			   <<'\n';
		output.close();

		//Set the flag to say that the leveller state file is already open for this run
		leveller_file_open_flag_ = true;
	}
	else {
		std::ofstream output; 
		output.open("leveller_state.csv",std::ios::app);
		if(output.is_open()) {printf("Leveller File opened correctly\n");}
		output << leveller.pitch_estimate_filtered_ << ',' << leveller.roll_estimate_filtered_ << ','
		 	   << leveller.acc0_latest_measurements_.x << ',' << leveller.acc0_latest_measurements_.y << ',' << leveller.acc0_latest_measurements_.z << ','
			   << leveller.acc1_latest_measurements_.x << ',' << leveller.acc1_latest_measurements_.y << ',' << leveller.acc1_latest_measurements_.z << ','
		 	   << leveller.acc2_latest_measurements_.x << ',' << leveller.acc2_latest_measurements_.y << ',' << leveller.acc2_latest_measurements_.z << ','
			   << leveller.acc_estimate_.x << ',' << leveller.acc_estimate_.y << ',' << leveller.acc_estimate_.z << ','
			   << leveller.actuator_velocity_target_.x << ',' << leveller.actuator_velocity_target_.y << ',' << leveller.actuator_velocity_target_.z 
			   <<'\n';
		output.close();
	}
}

void RobotDriver::WriteStabiliserStateToFile() {
	if(!stabiliser_file_open_flag_){
		//Create a new file and write the state to it
		std::ofstream readme_file;
		readme_file.open("README.txt",std::ios::out);
		readme_file << "target velocity x:  " << stabiliser.BFF_vel_reference_.x << "\n"
					<< "target velocity y:  " << stabiliser.BFF_vel_reference_.y << "\n"
					<< "target velocity s:  " << stabiliser.BFF_vel_reference_.z << "\n"
					<< "global gain:  " << stabiliser.global_gain_ << "\n";
		readme_file.close();

		std::ofstream output; 
		output.open("stabiliser_state.csv",std::ios::out);
		if(output.is_open()) {printf("Stabiliser File opened correctly\n");}
		output << "input_vel_x" << ',' << "input_vel_y" << ',' << "input_vel_z" << ','
			   << "input_vel_r" << ',' << "input_vel_p" << ',' << "input_vel_s" << ','
			   << "pos_measurement_x" << ',' << "pos_measurement_y" << ',' << "pos_measurement_z" << ','
			   << "pos_measurement_r" << ',' << "pos_measurement_p" << ',' << "pos_measurement_s" << ','
			   << "plat_vel_perturb_x" << ',' << "plat_vel_perturb_y" << ',' << "plat_vel_perturb_z" << ','
			   << "plat_vel_perturb_r" << ',' << "plat_vel_perturb_p" << ',' << "plat_vel_perturb_s" << ','
			   << "accelerometer0_x" << ',' << "accelerometer0_y" << ',' << "accelerometer0_z" << ','
			   << "accelerometer1_x" << ',' << "accelerometer1_y" << ',' << "accelerometer1_z" << ','
			   << "accelerometer2_x" << ',' << "accelerometer2_y" << ',' << "accelerometer2_z" << ','
			   << "accelerometer3_x" << ',' << "accelerometer3_y" << ',' << "accelerometer3_z" << ','
			   << "accelerometer4_x" << ',' << "accelerometer4_y" << ',' << "accelerometer4_z" << ','
			   << "accelerometer5_x" << ',' << "accelerometer5_y" << ',' << "accelerometer5_z" << ','
			   << "accelerometer0_ground_state_x" << ',' << "accelerometer0_ground_state_y" << ',' << "accelerometer0_ground_state_z" << ','
			   << "accelerometer1_ground_state_x" << ',' << "accelerometer1_ground_state_y" << ',' << "accelerometer1_ground_state_z" << ','
			   << "accelerometer2_ground_state_x" << ',' << "accelerometer2_ground_state_y" << ',' << "accelerometer2_ground_state_z" << ','
			   << "accelerometer3_ground_state_x" << ',' << "accelerometer3_ground_state_y" << ',' << "accelerometer3_ground_state_z" << ','
			   << "accelerometer4_ground_state_x" << ',' << "accelerometer4_ground_state_y" << ',' << "accelerometer4_ground_state_z" << ','
			   << "accelerometer5_ground_state_x" << ',' << "accelerometer5_ground_state_y" << ',' << "accelerometer5_ground_state_z" << ','
			   << "opto_pos_estimate_x" << ',' << "opto_pos_estimate_y" << ',' << "opto_pos_estimate_z" << ',' << "opto_pos_estimate_r" << ',' << "opto_pos_estimate_p" << ',' << "opto_pos_estimate_s" << ','
			   << "opto_vel_estimate_x" << ',' << "opto_vel_estimate_y" << ',' << "opto_vel_estimate_z" << ',' << "opto_vel_estimate_r" << ',' << "opto_vel_estimate_p" << ',' << "opto_vel_estimate_s" << ','
			   << "plat_pos_estimate_x" << ',' << "plat_pos_estimate_y" << ',' << "plat_pos_estimate_z" << ',' << "plat_pos_estimate_r" << ',' << "plat_pos_estimate_p" << ',' << "plat_pos_estimate_s" << ','
			   << "plat_vel_estimate_x" << ',' << "plat_vel_estimate_y" << ',' << "plat_vel_estimate_z" << ',' << "plat_vel_estimate_r" << ',' << "plat_vel_estimate_p" << ',' << "plat_vel_estimate_s" << ','
			   << "plat_acc_estimate_x" << ',' << "plat_acc_estimate_y" << ',' << "plat_acc_estimate_z" << ',' << "plat_acc_estimate_r" << ',' << "plat_acc_estimate_p" << ',' << "plat_acc_estimate_s" 
			   << '\n'

			   << stabiliser.input_velocity_.x << ',' << stabiliser.input_velocity_.y << ',' << stabiliser.input_velocity_.z << ','
			   << stabiliser.input_velocity_.r << ',' << stabiliser.input_velocity_.p << ',' << stabiliser.input_velocity_.s << ','
			   << stabiliser.platform_position_measurement_.x << ',' << stabiliser.platform_position_measurement_.y << ',' << stabiliser.platform_position_measurement_.z << ','
			   << stabiliser.platform_position_measurement_.r << ',' << stabiliser.platform_position_measurement_.p << ',' << stabiliser.platform_position_measurement_.s << ','
			   << stabiliser.plat_vel_perturbation_.x << ',' << stabiliser.plat_vel_perturbation_.y << ',' << stabiliser.plat_vel_perturbation_.z << ','
			   << stabiliser.plat_vel_perturbation_.r << ',' << stabiliser.plat_vel_perturbation_.p << ',' << stabiliser.plat_vel_perturbation_.s << ','
		 	   << stabiliser.acc0_latest_measurements_.x << ',' << stabiliser.acc0_latest_measurements_.y << ',' << stabiliser.acc0_latest_measurements_.z << ','
			   << stabiliser.acc1_latest_measurements_.x << ',' << stabiliser.acc1_latest_measurements_.y << ',' << stabiliser.acc1_latest_measurements_.z << ','
		 	   << stabiliser.acc2_latest_measurements_.x << ',' << stabiliser.acc2_latest_measurements_.y << ',' << stabiliser.acc2_latest_measurements_.z << ','
			   << stabiliser.acc3_latest_measurements_.x << ',' << stabiliser.acc3_latest_measurements_.y << ',' << stabiliser.acc3_latest_measurements_.z << ','
			   << stabiliser.acc4_latest_measurements_.x << ',' << stabiliser.acc4_latest_measurements_.y << ',' << stabiliser.acc4_latest_measurements_.z << ','
			   << stabiliser.acc5_latest_measurements_.x << ',' << stabiliser.acc5_latest_measurements_.y << ',' << stabiliser.acc5_latest_measurements_.z << ','
			   << stabiliser.acc0_ground_state_measurement_.x << ',' << stabiliser.acc0_ground_state_measurement_.y << ',' << stabiliser.acc0_ground_state_measurement_.z << ','
			   << stabiliser.acc1_ground_state_measurement_.x << ',' << stabiliser.acc1_ground_state_measurement_.y << ',' << stabiliser.acc1_ground_state_measurement_.z << ','
		 	   << stabiliser.acc2_ground_state_measurement_.x << ',' << stabiliser.acc2_ground_state_measurement_.y << ',' << stabiliser.acc2_ground_state_measurement_.z << ','
			   << stabiliser.acc3_ground_state_measurement_.x << ',' << stabiliser.acc3_ground_state_measurement_.y << ',' << stabiliser.acc3_ground_state_measurement_.z << ','
			   << stabiliser.acc4_ground_state_measurement_.x << ',' << stabiliser.acc4_ground_state_measurement_.y << ',' << stabiliser.acc4_ground_state_measurement_.z << ','
			   << stabiliser.acc5_ground_state_measurement_.x << ',' << stabiliser.acc5_ground_state_measurement_.y << ',' << stabiliser.acc5_ground_state_measurement_.z << ','
			   << stabiliser.opto_pos_estimate_.x << ',' << stabiliser.opto_pos_estimate_.y << ',' << stabiliser.opto_pos_estimate_.z << ',' << stabiliser.opto_pos_estimate_.r << ',' << stabiliser.opto_pos_estimate_.p << ',' << stabiliser.opto_pos_estimate_.s << ','
			   << stabiliser.opto_vel_estimate_.x << ',' << stabiliser.opto_vel_estimate_.y << ',' << stabiliser.opto_vel_estimate_.z << ',' << stabiliser.opto_vel_estimate_.r << ',' << stabiliser.opto_vel_estimate_.p << ',' << stabiliser.opto_vel_estimate_.s << ','
			   << stabiliser.plat_pos_estimate_.x << ',' << stabiliser.plat_pos_estimate_.y << ',' << stabiliser.plat_pos_estimate_.z << ',' << stabiliser.plat_pos_estimate_.r << ',' << stabiliser.plat_pos_estimate_.p << ',' << stabiliser.plat_pos_estimate_.s << ','
			   << stabiliser.plat_vel_estimate_.x << ',' << stabiliser.plat_vel_estimate_.y << ',' << stabiliser.plat_vel_estimate_.z << ',' << stabiliser.plat_vel_estimate_.r << ',' << stabiliser.plat_vel_estimate_.p << ',' << stabiliser.plat_vel_estimate_.s << ','
			   << stabiliser.plat_acc_estimate_.x << ',' << stabiliser.plat_acc_estimate_.y << ',' << stabiliser.plat_acc_estimate_.z << ',' << stabiliser.plat_acc_estimate_.r << ',' << stabiliser.plat_acc_estimate_.p << ',' << stabiliser.plat_acc_estimate_.s 
			   <<'\n';
		output.close();

		//Set the flag to say that the leveller state file is already open for this run
		stabiliser_file_open_flag_ = true;
	}
	else {
		std::ofstream output; 
		output.open("stabiliser_state.csv",std::ios::app);
		if(output.is_open()) {printf("Stabiliser File opened correctly\n");}
		output << stabiliser.input_velocity_.x << ',' << stabiliser.input_velocity_.y << ',' << stabiliser.input_velocity_.z << ','
			   << stabiliser.input_velocity_.r << ',' << stabiliser.input_velocity_.p << ',' << stabiliser.input_velocity_.s << ','
			   << stabiliser.platform_position_measurement_.x << ',' << stabiliser.platform_position_measurement_.y << ',' << stabiliser.platform_position_measurement_.z << ','
			   << stabiliser.platform_position_measurement_.r << ',' << stabiliser.platform_position_measurement_.p << ',' << stabiliser.platform_position_measurement_.s << ','
			   << stabiliser.plat_vel_perturbation_.x << ',' << stabiliser.plat_vel_perturbation_.y << ',' << stabiliser.plat_vel_perturbation_.z << ','
			   << stabiliser.plat_vel_perturbation_.r << ',' << stabiliser.plat_vel_perturbation_.p << ',' << stabiliser.plat_vel_perturbation_.s << ','
		 	   << stabiliser.acc0_latest_measurements_.x << ',' << stabiliser.acc0_latest_measurements_.y << ',' << stabiliser.acc0_latest_measurements_.z << ','
			   << stabiliser.acc1_latest_measurements_.x << ',' << stabiliser.acc1_latest_measurements_.y << ',' << stabiliser.acc1_latest_measurements_.z << ','
		 	   << stabiliser.acc2_latest_measurements_.x << ',' << stabiliser.acc2_latest_measurements_.y << ',' << stabiliser.acc2_latest_measurements_.z << ','
			   << stabiliser.acc3_latest_measurements_.x << ',' << stabiliser.acc3_latest_measurements_.y << ',' << stabiliser.acc3_latest_measurements_.z << ','
			   << stabiliser.acc4_latest_measurements_.x << ',' << stabiliser.acc4_latest_measurements_.y << ',' << stabiliser.acc4_latest_measurements_.z << ','
			   << stabiliser.acc5_latest_measurements_.x << ',' << stabiliser.acc5_latest_measurements_.y << ',' << stabiliser.acc5_latest_measurements_.z << ','
			   << stabiliser.acc0_ground_state_measurement_.x << ',' << stabiliser.acc0_ground_state_measurement_.y << ',' << stabiliser.acc0_ground_state_measurement_.z << ','
			   << stabiliser.acc1_ground_state_measurement_.x << ',' << stabiliser.acc1_ground_state_measurement_.y << ',' << stabiliser.acc1_ground_state_measurement_.z << ','
		 	   << stabiliser.acc2_ground_state_measurement_.x << ',' << stabiliser.acc2_ground_state_measurement_.y << ',' << stabiliser.acc2_ground_state_measurement_.z << ','
			   << stabiliser.acc3_ground_state_measurement_.x << ',' << stabiliser.acc3_ground_state_measurement_.y << ',' << stabiliser.acc3_ground_state_measurement_.z << ','
			   << stabiliser.acc4_ground_state_measurement_.x << ',' << stabiliser.acc4_ground_state_measurement_.y << ',' << stabiliser.acc4_ground_state_measurement_.z << ','
			   << stabiliser.acc5_ground_state_measurement_.x << ',' << stabiliser.acc5_ground_state_measurement_.y << ',' << stabiliser.acc5_ground_state_measurement_.z << ','
			   << stabiliser.opto_pos_estimate_.x << ',' << stabiliser.opto_pos_estimate_.y << ',' << stabiliser.opto_pos_estimate_.z << ',' << stabiliser.opto_pos_estimate_.r << ',' << stabiliser.opto_pos_estimate_.p << ',' << stabiliser.opto_pos_estimate_.s << ','
			   << stabiliser.opto_vel_estimate_.x << ',' << stabiliser.opto_vel_estimate_.y << ',' << stabiliser.opto_vel_estimate_.z << ',' << stabiliser.opto_vel_estimate_.r << ',' << stabiliser.opto_vel_estimate_.p << ',' << stabiliser.opto_vel_estimate_.s << ','
			   << stabiliser.plat_pos_estimate_.x << ',' << stabiliser.plat_pos_estimate_.y << ',' << stabiliser.plat_pos_estimate_.z << ',' << stabiliser.plat_pos_estimate_.r << ',' << stabiliser.plat_pos_estimate_.p << ',' << stabiliser.plat_pos_estimate_.s << ','
			   << stabiliser.plat_vel_estimate_.x << ',' << stabiliser.plat_vel_estimate_.y << ',' << stabiliser.plat_vel_estimate_.z << ',' << stabiliser.plat_vel_estimate_.r << ',' << stabiliser.plat_vel_estimate_.p << ',' << stabiliser.plat_vel_estimate_.s << ','
			   << stabiliser.plat_acc_estimate_.x << ',' << stabiliser.plat_acc_estimate_.y << ',' << stabiliser.plat_acc_estimate_.z << ',' << stabiliser.plat_acc_estimate_.r << ',' << stabiliser.plat_acc_estimate_.p << ',' << stabiliser.plat_acc_estimate_.s 
			   <<'\n';
		output.close();
	}
}

/*
RESONANCE TESTING
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


//Applies a sinusoidal velocity in a certain direction at a certain frequency for some number of seconds
//We update the velocity each millisecond to ensure smooth operation of the motors
//We also request the accelerations and store them in a file named by the relevant frequency
void RobotDriver::ApplySinusoidalVelocity(Servo::Doubles velocity_amplitude, 
										  double amplitude_target,
										  unsigned int frequency, 
										  unsigned int time,
										  char sweep_class)	{
	unsigned int time_counter = 0; //counts the time (somewhat inaccurately) since the start of the subroutine, in milliseconds
	Servo::Doubles velocity_temp;

	while(time_counter < 1000*(time+1)) {
		double temp = cos(2.0*PI*(frequency/1000.0)*(time_counter/1000.0)); //cos(2PIft)
		velocity_temp.x = temp*velocity_amplitude.x;//x
		velocity_temp.y = temp*velocity_amplitude.y;//y
		velocity_temp.z = temp*velocity_amplitude.z;//yaw, (this is for specialist purposes and should usually be zero)

		UpdateBFFVelocity(velocity_temp);
		RequestAccelerations();
		teensy_port.PacketManager();
		usleep(500);
		if(time_counter>=1000) {WriteSinusoidalAccelerationsToFile(frequency,amplitude_target,sweep_class);} //We wait 1 second to dampen any 
																				//transient behaviour before recording
		

		time_counter += 1;
	}
	RequestAllStop();
}

//Applies a sinusoidal sweep in the x direction
//We apply each sinusoid for 30 seconds
//Note that the only valid direction options are 'x','y','z' (and that 'z' refers to a yaw rotation)
void RobotDriver::SinusoidalSweepLinear(unsigned int start_frequency, 
										unsigned int end_frequency, 
										unsigned int step_size,
										unsigned int power,
										unsigned int time,
										char direction,
										double max_amplitude)	{
	Servo::Doubles target;
	target.x = 0.0;	 
	target.y = 0.0;
	target.z = 0.0;
	for(unsigned int current_frequency = start_frequency; current_frequency <= end_frequency; current_frequency += step_size){
		double temp = max_amplitude*pow(start_frequency/(current_frequency*1.0),1.0/power);//We scale the target velocity down by 1/root_pow(f)
		switch(direction){
			case 'x':
				target.x = temp;
				break;
			case 'y':
				target.y = temp;
				break;
			case 'z':
				target.z = temp;
				break;
			default:
				printf("Direction Invalid\n");
		}
		ApplySinusoidalVelocity(target,temp,current_frequency,time,'f');
	}
}

//Slowly increases the magnitude of a sinusoid and stores the measured accelerations
//Note that the only valid direction options are 'x','y','z' (and that 'z' refers to a yaw rotation)
void RobotDriver::SinusoidalAmplitudeSweepLinear(unsigned int frequency,
                                                double amp_min,
                                                double amp_max,
                                                double sample_count,
                                                unsigned int time,
                                                char direction)	{
	Servo::Doubles target;
	target.x = 0.0;	 
	target.y = 0.0;
	target.z = 0.0;
	for(double amplitude = amp_min; amplitude <= amp_max; amplitude += (amp_max-amp_min)/sample_count){
		double temp = amplitude;//We scale the target velocity down by 1/root_pow(f)
		switch(direction){
			case 'x':
				target.x = temp;
				break;
			case 'y':
				target.y = temp;
				break;
			case 'z':
				target.z = temp;
				break;
			default:
				printf("Direction Invalid\n");
		}
		ApplySinusoidalVelocity(target,temp,frequency,time,'a');
	}
}

void RobotDriver::SinusoidalSweepLog(unsigned int start_frequency, 
									 unsigned int end_frequency, 
									 double factor,
									 unsigned int power,
									 unsigned int time,
									 char direction,
									 double max_amplitude)	{
	Servo::Doubles target;
	target.x = 0.0;
	target.y = 0.0;
	target.z = 0.0;
	for(double current_frequency = start_frequency*1.0; current_frequency <= end_frequency; current_frequency *= factor){
		double temp = max_amplitude*pow(start_frequency/current_frequency,1.0/power);//We scale the target velocity down by 1/root_pow(f)
		switch(direction){
			case 'x':
				target.x = temp;
				break;
			case 'y':
				target.y = temp;
				break;
			case 'z':
				target.z = temp;
				break;
			default:
				printf("Direction Invalid\n");
		}
		ApplySinusoidalVelocity(target,temp,lrint(current_frequency),time,'f');
	}
}

//Writes the accelerations to a file which is named for easy passing when conducting resonance tests
//Note that the only acceptable sweep_class values are 'a' and 'f' corresponding to amplitude
//and frequency sweeps respectively. (N.B, this could be generalised but it is not currently necessary)
void RobotDriver::WriteSinusoidalAccelerationsToFile(unsigned int frequency,double amplitude,char sweep_class) {
	char title_buffer[50];
	switch(sweep_class) {
		case 'f':
			sprintf(title_buffer,"%d_milliHertz_data.csv",frequency);
			break;
		case 'a':
			sprintf(title_buffer,"%f_amplitude_%d_milliHertz_data.csv",amplitude,frequency);
			break;
		default:
			printf("sweep class invalid");
	}
	if(!sinusoidal_file_open_flag_){
		//Create a new file and write the state to it
		std::ofstream output; 
		output.open(title_buffer,std::ios::out);
		if(output.is_open()) {printf("File opened correctly\n");}
		output << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.x[0],teensy_port.accelerometer3_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.y[0],teensy_port.accelerometer3_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.x[0],teensy_port.accelerometer4_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.y[0],teensy_port.accelerometer4_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.x[0],teensy_port.accelerometer5_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.y[0],teensy_port.accelerometer5_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1])  
		 	   <<'\n';
		output.close();

		//Set the flag to say that the leveller state file is already open for this run
		sinusoidal_file_open_flag_ = true;
		
	}
	else {
		std::ofstream output; 
		output.open(title_buffer,std::ios::app);
		if(output.is_open()) {printf("File opened correctly\n");}
		output << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.x[0],teensy_port.accelerometer3_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.y[0],teensy_port.accelerometer3_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.x[0],teensy_port.accelerometer4_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.y[0],teensy_port.accelerometer4_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.x[0],teensy_port.accelerometer5_in_.x[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.y[0],teensy_port.accelerometer5_in_.y[1]) << ',' 
			   << AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1])  
		 	   <<'\n';
		output.close();
	}
}


/*
PRIVATE DEBUGGING
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Debugging function to have the runtime reported
void RobotDriver::PrintRuntime() {
    printf("Longest Runtime is %u\n",BytesTouInt(teensy_port.runtime_in_[0],
                                                 teensy_port.runtime_in_[1],
                                                 teensy_port.runtime_in_[2],
                                                 teensy_port.runtime_in_[3]));
}

//Debugging function to have all of the accelerations reported
void RobotDriver::PrintAccelerations() {
    printf("Last Accelerometer 0 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]));
    printf("Last Accelerometer 0 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]));
    printf("Last Accelerometer 0 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]));
    printf("Last Accelerometer 1 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]));
    printf("Last Accelerometer 1 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]));
    printf("Last Accelerometer 1 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]));
    printf("Last Accelerometer 2 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]));
    printf("Last Accelerometer 2 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]));
    printf("Last Accelerometer 2 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]));
    printf("Last Accelerometer 3 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.x[0],teensy_port.accelerometer3_in_.x[1]));
    printf("Last Accelerometer 3 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.y[0],teensy_port.accelerometer3_in_.y[1]));
    printf("Last Accelerometer 3 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer3_in_.z[0],teensy_port.accelerometer3_in_.z[1]));
    printf("Last Accelerometer 4 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.x[0],teensy_port.accelerometer4_in_.x[1]));
    printf("Last Accelerometer 4 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.y[0],teensy_port.accelerometer4_in_.y[1]));
    printf("Last Accelerometer 4 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer4_in_.z[0],teensy_port.accelerometer4_in_.z[1]));
    printf("Last Accelerometer 5 x reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.x[0],teensy_port.accelerometer5_in_.x[1]));
    printf("Last Accelerometer 5 y reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.y[0],teensy_port.accelerometer5_in_.y[1]));
    printf("Last Accelerometer 5 z reading was %f\n",
            AccelerationBytesToPhysicalDouble(teensy_port.accelerometer5_in_.z[0],teensy_port.accelerometer5_in_.z[1]));
}

void RobotDriver::PrintStepCounts() {
	printf("Last Motor 0 Step Count was %d\n",navigator.current_step_count_.motor_0);
	printf("Last Motor 1 Step Count was %d\n",navigator.current_step_count_.motor_1);
	printf("Last Motor 2 Step Count was %d\n",navigator.current_step_count_.motor_2);
	printf("Last Actuator 0 Step Count was %d\n",leveller.current_step_count_.motor_0);
	printf("Last Actuator 1 Step Count was %d\n",leveller.current_step_count_.motor_1);
	printf("Last Actuator 2 Step Count was %d\n",leveller.current_step_count_.motor_2);
}

#include <chrono>

bool closed_loop_enable_flag = true;

int main() {
	//Necessary global timing measures
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::microseconds;

	auto time_point_start = high_resolution_clock::now();
	auto time_point_current = high_resolution_clock::now();
	auto last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_leveller_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_leveller_subtimepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_navigator_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

	RobotDriver driver;
	
	driver.StabiliserTest();
	driver.StabiliserSetup();
	//driver.EngageLeveller();
	//driver.EngageNavigator();

	//Open loop tests
	//driver.SinusoidalSweepLog(10000,100000,1.0046,1,3,'y',0.002);
	//driver.SinusoidalAmplitudeSweepLinear(1000,0,0.015,100,5,'y');

    //driver.RaiseAndLowerTest();
    //driver.LinearSweepTest();
	//driver.MeasureOrientationMeasurementNoise();
	//driver.NavigatorTest();
	//driver.StabiliserTest();

	//We rest the start clock and start the closed loop operation
	time_point_start = high_resolution_clock::now();
	while(closed_loop_enable_flag) {
		//Boolean to store if we have done anything on this loop and wait a little bit if we haven't
		bool controller_active = false;


		time_point_current = high_resolution_clock::now();
		global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

		if(global_timepoint-last_stabiliser_timepoint > 1000) {
			printf("%ld\n",global_timepoint-last_stabiliser_timepoint);
			last_stabiliser_timepoint = global_timepoint;
			if(driver.stabiliser.enable_flag_) {
				controller_active = true;
				driver.StabiliserLoop();
			}
		}

		if((global_timepoint-last_leveller_timepoint) > 10000) {
			last_leveller_timepoint = global_timepoint;
			if(driver.leveller.enable_flag_) {
				controller_active = true;
				driver.LevellerLoop();	
			}
		}
		else if(global_timepoint-last_leveller_subtimepoint > 1000) {
			last_leveller_subtimepoint = global_timepoint;
			if(driver.leveller.enable_flag_) {
				controller_active = true;
				driver.LevellerSubLoop();	
			}
		}
		
		if((global_timepoint-last_navigator_timepoint) > 10000) {
			last_navigator_timepoint = global_timepoint;
			if(driver.navigator.enable_flag_) {
				controller_active = true;
				driver.NavigatorLoop();
			}
		}

		//If the leveller is only enables for a short run after 20 seconds we disable it
		//This is for the stabiliser runs
		if(driver.short_level_flag2_){
			if(global_timepoint > 10000000){
				driver.SetNewTargetAngle(driver.leveller.pitch_target_cache_,driver.leveller.roll_target_cache_);
				driver.RequestResetStepCount();
				driver.short_level_flag2_ = false;
			}
		}

		if(driver.short_level_flag_){
			if(global_timepoint > 20000000) {
				controller_active = true;
				driver.EngageStabiliser();

			}
		}

		if (!controller_active) {
			usleep(10);
		}

		driver.teensy_port.PacketManager();
	}

	driver.teensy_port.ClosePort();

	return -1;
}


