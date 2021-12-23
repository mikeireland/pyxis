//Controller.cpp
#include "RobotDriver.h"

using namespace Control;

//Ramps the velocity of the deputy up linear from zero to the velocity target from 
//all stop over time seconds
void RobotDriver::LinearLateralRamp(Servo::VelDoubles velocity_target, double time) {
    Servo::VelDoubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.x = velocity_scale*velocity_target.x; 
        velocity_target_wrapper.y = velocity_scale*velocity_target.y; 
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.WriteMessage();
        usleep(1000);     
    }
}


//Ramps the yaw velocity of the deputy up linear from zero to the velocity target from 
//all stop over time seconds
void RobotDriver::LinearYawRamp(double yaw_rate_target, double time) {
    Servo::VelDoubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.z = velocity_scale*yaw_rate_target;  
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.WriteMessage();
        usleep(1000);     
    }
}

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
	teensy_port.AddToPacket(SetRaw3);	
	teensy_port.AddToPacket(SetRaw4);
	teensy_port.AddToPacket(SetRaw5);	
}

//Sets the actuators to some velocity (this is intended for use in the leveling servo loop and si is designed to pitch and roll)
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::UpdateActuatorVelocity(Servo::VelDoubles velocity) {
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
	teensy_port.AddToPacket(SetRaw3);	
	teensy_port.AddToPacket(SetRaw4);
	teensy_port.AddToPacket(SetRaw5);	
}

//Ramps the actuators from one velocity to another over some time period
void RobotDriver::LinearActuatorRamp(Servo::VelDoubles velocity_initial_,Servo::VelDoubles velocity_target, double time) {
    Servo::VelDoubles velocity_target_temp;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
		velocity_target_temp.x = velocity_initial_.x+velocity_scale*(velocity_target.x-velocity_initial_.x); 
    	velocity_target_temp.y = velocity_initial_.y+velocity_scale*(velocity_target.y-velocity_initial_.y); 
		velocity_target_temp.z = velocity_initial_.z+velocity_scale*(velocity_target.z-velocity_initial_.z); 
        UpdateActuatorVelocity(velocity_target_temp);
        teensy_port.WriteMessage();
        usleep(1000);     
    }
}

//Requests an update of the Body Fixed Frame velocity of the robot
//Note that the message still needs to be sent after this routine is run, it will not send it on its own
void RobotDriver::UpdateBFFVelocity(Servo::VelDoubles velocities) {
    //Write the new velocity as the velocity target
    BFF_velocity_target_ = velocities;

    //We then convert from the target BFF velocitities to the 
    //read velocity of each of the motors 
    motor_velocities_target_.x = -BFF_velocity_target_.y - BFF_velocity_target_.z;
    motor_velocities_target_.y = ( BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    motor_velocities_target_.z = (-BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    //For now the Yawing is not included in the above but will need to be added

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
    teensy_port.AddToPacket(SetRaw0);
    teensy_port.AddToPacket(SetRaw1);
    teensy_port.AddToPacket(SetRaw2);
}

//Pull all of the accelerations from the device (21 Bytes of return data)
void RobotDriver::RequestAccelerations() {
    teensy_port.AddToPacket(Acc0Wr);
    teensy_port.AddToPacket(Acc1Wr);
    teensy_port.AddToPacket(Acc2Wr);
    teensy_port.AddToPacket(Acc3Wr);
    teensy_port.AddToPacket(Acc4Wr);
    teensy_port.AddToPacket(Acc5Wr);
}

//Set the motor velocities to zero
void RobotDriver::RequestAllStop() {
    teensy_port.AddToPacket(STOP);
    BFF_velocity_target_.x = 0;
    BFF_velocity_target_.y = 0;
    BFF_velocity_target_.z = 0;
    motor_velocities_target_.x = 0;
    motor_velocities_target_.y = 0;
    motor_velocities_target_.z = 0;
    teensy_port.WriteMessage();
    usleep(1000);
}

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

//Command the robot to linearly sweep up to its maximum velocity 
//we also occasionally read off the accelerations as a debugging aid
void RobotDriver::LinearSweepTest() {
    Servo::VelDoubles vel_target_wrapper;

    //Speed up in the x direction and then reverse
    vel_target_wrapper.x = 0.014/1.414;
    vel_target_wrapper.y = 0.014/1.414;
    LinearLateralRamp(vel_target_wrapper,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(1000);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations();    
    RequestAllStop();   

    vel_target_wrapper.x = -0.014/1.414;
    vel_target_wrapper.y = -0.014/1.414;
    LinearLateralRamp(vel_target_wrapper,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(1000);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations();  
    RequestAllStop();

    LinearYawRamp(0.014,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(1000);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();

    LinearYawRamp(-0.014,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(1000);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();
}

void RobotDriver::RaiseAndLowerTest() {
	RequestAllStop();
	
	UpdateZVelocity(0.001);
	RequestAccelerations();
	teensy_port.AddToPacket(RUNTIME);
	teensy_port.WriteMessage();
	usleep(1000);
	teensy_port.ReadMessage();
	PrintRuntime();
	PrintAccelerations();
	sleep(10);
	RequestAllStop();
	

	UpdateZVelocity(-0.001);
	RequestAccelerations();
	teensy_port.AddToPacket(RUNTIME);
	teensy_port.WriteMessage();
	usleep(1000);
	teensy_port.ReadMessage();
	PrintRuntime();
	PrintAccelerations();
	sleep(10);

	RequestAllStop();
}

//Converts the last incoming accelerometer values into doubles and passes them to the Leveller
void RobotDriver::PassAccelBytesToLeveller() {
	leveller.acc0_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.x[0],teensy_port.accelerometer0_in_.x[1]);
	leveller.acc0_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.y[0],teensy_port.accelerometer0_in_.y[1]);
	leveller.acc0_latest_measurements_.z = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer0_in_.z[0],teensy_port.accelerometer0_in_.z[1]);
	leveller.acc1_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.x[0],teensy_port.accelerometer1_in_.x[1]);
	leveller.acc1_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.y[0],teensy_port.accelerometer1_in_.y[1]);
	leveller.acc1_latest_measurements_.z = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer1_in_.z[0],teensy_port.accelerometer1_in_.z[1]);
	leveller.acc2_latest_measurements_.x = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.x[0],teensy_port.accelerometer2_in_.x[1]);
	leveller.acc2_latest_measurements_.y = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.y[0],teensy_port.accelerometer2_in_.y[1]);
	leveller.acc2_latest_measurements_.z = AccelerationBytesToPhysicalDouble(teensy_port.accelerometer2_in_.z[0],teensy_port.accelerometer2_in_.z[1]);
}

void RobotDriver::EngageLeveller() {
	while(true) {
		RequestAccelerations();
		teensy_port.WriteMessage();
		usleep(1000);
		teensy_port.ReadMessage();
		PassAccelBytesToLeveller();
		leveller.UpdateTarget();
		LinearActuatorRamp(leveller.last_actuator_velocity_target_,leveller.actuator_velocity_target_,0.05);
		usleep(1000);
		WriteLevellerStateToFile();
		usleep(48000);
	}
}

void RobotDriver::MeasureOrientationMeasurementNoise() {
	for(int i = 0; i < 10000; ++i) {
		//Each millisecond we request the current accelerometer measurements
		RequestAccelerations();
		teensy_port.WriteMessage();
		usleep(500);
		teensy_port.ReadMessage();

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
		if(output.is_open()) {printf("File opened correctly\n");}
		output << leveller.pitch_estimate_ << ',' << leveller.roll_estimate_ << ','
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
		if(output.is_open()) {printf("File opened correctly\n");}
		output << leveller.pitch_estimate_ << ',' << leveller.roll_estimate_ << ','
		 	   << leveller.acc0_latest_measurements_.x << ',' << leveller.acc0_latest_measurements_.y << ',' << leveller.acc0_latest_measurements_.z << ','
			   << leveller.acc1_latest_measurements_.x << ',' << leveller.acc1_latest_measurements_.y << ',' << leveller.acc1_latest_measurements_.z << ','
		 	   << leveller.acc2_latest_measurements_.x << ',' << leveller.acc2_latest_measurements_.y << ',' << leveller.acc2_latest_measurements_.z << ','
			   << leveller.acc_estimate_.x << ',' << leveller.acc_estimate_.y << ',' << leveller.acc_estimate_.z << ','
			   << leveller.actuator_velocity_target_.x << ',' << leveller.actuator_velocity_target_.y << ',' << leveller.actuator_velocity_target_.z 
			   <<'\n';
		output.close();
	}
}

int main() {
    RobotDriver driver;
	//driver.RaiseAndLowerTest();
    //driver.LinearSweepTest();
	driver.EngageLeveller();
	//driver.MeasureOrientationMeasurementNoise();
    driver.teensy_port.ClosePort();
    return 0;
}


