//Controller.cpp
#include "RobotDriver.h"

// C library headers
#include <stdio.h>
#include <string.h>
#include <math.h>
#define PI 3.14159265

// Linux headers
#include <unistd.h> // write(), read(), close()

//Macro headers
#include "Commands.h"
#include "ErrorCodes.h"
#include "Decode.h"

using namespace Control;

//Ramps the velocity up of robot up linear from zero to the velocity target from 
//all stop over time seconds
void RobotDriver::LinearLateralRamp(VelDoubles velocity_target, double time) {
    VelDoubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.x = velocity_scale*velocity_target.x; 
        velocity_target_wrapper.y = velocity_scale*velocity_target.y; 
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.WriteMessage();
        usleep(1000);     
    }
}

void RobotDriver::LinearYawRamp(double yaw_rate_target, double time) {
    VelDoubles velocity_target_wrapper;
    RequestAllStop();  

    for(double velocity_scale = 0; velocity_scale < 1; velocity_scale += 1/(time*1000)){
        velocity_target_wrapper.z = velocity_scale*yaw_rate_target;  
        UpdateBFFVelocity(velocity_target_wrapper);
        teensy_port.WriteMessage();
        usleep(1000);     
    }
}

//Command the robot to linearly sweep up to its maximum velocity while reading off the accelerations
void RobotDriver::LinearSweep() {
    VelDoubles vel_target_wrapper;

    //Speed up in the x direction and then reverse
    vel_target_wrapper.x = 0.014/1.414;
    vel_target_wrapper.y = 0.014/1.414;
    LinearLateralRamp(vel_target_wrapper,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(500);
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
    usleep(500);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations();  
    RequestAllStop();

    LinearYawRamp(0.014,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(500);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();

    LinearYawRamp(-0.014,10);
    RequestAccelerations();
    teensy_port.AddToPacket(RUNTIME);
    teensy_port.WriteMessage();
    usleep(500);
    teensy_port.ReadMessage();
    PrintRuntime();
    PrintAccelerations(); 
    RequestAllStop();
}

//Requests an update of the Body Fixed Frame velocity of the robot
void RobotDriver::UpdateBFFVelocity(VelDoubles velocities) {
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

int main() {
    RobotDriver driver;
    driver.LinearSweep();
    driver.teensy_port.ClosePort();
    return 0;
}
