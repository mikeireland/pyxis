//Servo.cpp
#include "Servo.h"
#include <math.h>
#include <stdio.h>

using namespace Servo;

/*
DEFINITIONS FOR THE Leveller OBJECT
*/

//Set a saturation velocity of 1000 micron/s
double Leveller::saturation_velocity_ = 0.001;

//Function to update Leveller's target velocitites
//TODO Add a Kalman filter to better estimate the state
void Leveller::UpdateTarget() {
    last_actuator_velocity_target_ = actuator_velocity_target_;
    CombineAccelerations();
    EstimateState();
    ApplyLQRGain();
    ApplySaturationFilter();
}

//Here we combine the three accelerometer measurements into a single
//measurement of the overall acceleration (this is likely not final)
void Leveller::CombineAccelerations() {
    acc_estimate_.x = (-0.5*acc0_latest_measurements_.x+0.866*acc0_latest_measurements_.y
                           +acc1_latest_measurements_.x
                       -0.5*acc2_latest_measurements_.x-0.866*acc2_latest_measurements_.y)/3.0;

    acc_estimate_.y = (-0.866*acc0_latest_measurements_.x-0.5*acc0_latest_measurements_.y
                                                             +acc1_latest_measurements_.y
                       +0.866*acc2_latest_measurements_.x-0.5*acc2_latest_measurements_.y)/3.0;

    //We flip the sign on the z component so that gravity is measured downwards
    acc_estimate_.z = -(acc0_latest_measurements_.z
                       +acc1_latest_measurements_.z
                       +acc2_latest_measurements_.z)/3.0;
}

//Compute the pitch and roll given the value of the accelerations
void Leveller::EstimateState() {
    pitch_estimate_ =  atan(-acc_estimate_.x/sqrt(acc_estimate_.z*acc_estimate_.z+
                                                  acc_estimate_.y*acc_estimate_.y))*180/PI;

    roll_estimate_ = -atan(acc_estimate_.y/acc_estimate_.z)*180/PI;
    printf("Pitch is %f\n",pitch_estimate_);
    printf("Roll is %f\n",roll_estimate_);
}

void Leveller::ApplyLQRGain() {
    actuator_velocity_target_.x = -0.0001*(3.5*pitch_estimate_-8*roll_estimate_);
    actuator_velocity_target_.y = -0.0001*(-7*pitch_estimate_);
    actuator_velocity_target_.z = -0.0001*(3.5*pitch_estimate_+8*roll_estimate_);
}

//Saturate the velocity if it is too high
void Leveller::ApplySaturationFilter() {
    if(actuator_velocity_target_.x > saturation_velocity_) {actuator_velocity_target_.x = saturation_velocity_; printf("Saturated Actuator 0\n");}
    else if(actuator_velocity_target_.x < -saturation_velocity_) {actuator_velocity_target_.x = -saturation_velocity_; printf("Saturated Actuator 0\n");}

    if(actuator_velocity_target_.y > saturation_velocity_) {actuator_velocity_target_.y = saturation_velocity_; printf("Saturated Actuator 1\n");}
    else if(actuator_velocity_target_.y < -saturation_velocity_) {actuator_velocity_target_.y = -saturation_velocity_; printf("Saturated Actuator 1\n");}

    if(actuator_velocity_target_.z > saturation_velocity_) {actuator_velocity_target_.z = saturation_velocity_;printf("Saturated Actuator 2\n");}
    else if(actuator_velocity_target_.z < -saturation_velocity_) {actuator_velocity_target_.z = -saturation_velocity_; printf("Saturated Actuator 2\n");}
}

/*
DEFINITIONS FOR THE Navigator OBJECT
*/

//We define the following relevant quantities for the Navigator controller 
double Navigator::saturation_velocity_ = 0.002;//0.015;
double Navigator::gain_x_ = 0.1;
double Navigator::gain_y_ = 0.1;
double Navigator::gain_yaw_ = 0.1;


//Update the target velocity of the motors based on the latest step count
//We do this with a simple proportional gain feedback loop. A full PID controller will probably be needed
//where precision is required but for purely slewing purposes this is likely sufficient. 
//For now there will be now yawing allowed. This may need to be added later as a separate direction controller.
void Navigator::UpdateTarget() {
    ComputeState();
    ApplyPropGain();
    ApplySaturationFilter();
}

//Convert the step count into a physical measurement of the distance moved in m
void Navigator::ComputeState() {
    //This transformation is the inverse of that used to compute the motor velocities from the BFF velocities 
    //The matrix inverse was computed in Mathematica
    double temp_x   =                                     0.5774*current_step_count_.motor_1-0.5774*current_step_count_.motor_2;//x displacement
    double temp_y   = -0.6667*current_step_count_.motor_0+0.3333*current_step_count_.motor_1+0.3333*current_step_count_.motor_2;//y displacement
    double temp_yaw = -0.3333*current_step_count_.motor_0-0.3333*current_step_count_.motor_1-0.3333*current_step_count_.motor_2;//yaw displacement

    //Note that the above are being stored in units of microsteps. To convert to metres we note that there are 295nm in a microstep 
    //of the motors and therefore that we have
    distance_from_reference_.x = temp_x*0.000000297;
    distance_from_reference_.y = temp_y*0.000000297;
    //For the yaw we must also multiply by the rotational factor to recognise that our yaw is a dimensionless quantity in radians
    //Note that the 0.3 below represents a 30cm radius for the rotation. This is just a ballpark figure and will need to 
    //be made more accurate for any precision purposes
    distance_from_reference_.z = temp_yaw*(0.000000297/0.3);
}

void Navigator::ApplyPropGain() {
    //We apply a proportional gain to compute target Body Fixed Frame velocities
    BFF_velocity_target_.x = -gain_x_*(distance_from_reference_.x-position_target_.x);
    BFF_velocity_target_.y = -gain_y_*(distance_from_reference_.y-position_target_.y);
    BFF_velocity_target_.z = -gain_yaw_*(distance_from_reference_.z-position_target_.z);

    //We then convert from the target BFF velocitities to the 
    //read velocity of each of the motors 
    motor_velocity_target_.x = -BFF_velocity_target_.y - BFF_velocity_target_.z;
    motor_velocity_target_.y = ( BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    motor_velocity_target_.z = (-BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;

}

void Navigator::ApplySaturationFilter() {
    if(motor_velocity_target_.x > saturation_velocity_) {motor_velocity_target_.x = saturation_velocity_; printf("Saturated Motor 0\n");}
    else if(motor_velocity_target_.x < -saturation_velocity_) {motor_velocity_target_.x = -saturation_velocity_; printf("Saturated Motor 0\n");}

    if(motor_velocity_target_.y > saturation_velocity_) {motor_velocity_target_.y = saturation_velocity_; printf("Saturated Motor 1\n");}
    else if(motor_velocity_target_.y < -saturation_velocity_) {motor_velocity_target_.y = -saturation_velocity_; printf("Saturated Motor 1\n");}

    if(motor_velocity_target_.z > saturation_velocity_) {motor_velocity_target_.z = saturation_velocity_;printf("Saturated Motor 2\n");}
    else if(motor_velocity_target_.z < -saturation_velocity_) {motor_velocity_target_.z = -saturation_velocity_; printf("Saturated Motor 2\n");}
}

void Navigator::SetNewPosition(Doubles position_target) {
    distance_from_reference_.x = 0;
    distance_from_reference_.y = 0;
    distance_from_reference_.z = 0;
    BFF_velocity_target_.x = 0;
    BFF_velocity_target_.y = 0;
    BFF_velocity_target_.z = 0;
    motor_velocity_target_.x = 0;
    motor_velocity_target_.y = 0;
    motor_velocity_target_.z = 0;

    position_target_ = position_target;
}



