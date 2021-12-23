//Servo.cpp
#include "Servo.h"
#include <math.h>
#include <stdio.h>

using namespace Servo;

//Set a saturation velocity of 500 micron/s
double Leveller::saturation_velocity_ = 0.00005;

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

    roll_estimate_ = atan(acc_estimate_.y/acc_estimate_.z)*180/PI;
    printf("Pitch is %f\n",pitch_estimate_);
    printf("Roll is %f\n",roll_estimate_);
}

void Leveller::ApplyLQRGain() {
    actuator_velocity_target_.x = -0.00001*(3.5*pitch_estimate_-8*roll_estimate_);
    actuator_velocity_target_.y = -0.00001*(-7*pitch_estimate_);
    actuator_velocity_target_.z = -0.00001*(3.5*pitch_estimate_+8*roll_estimate_);
}

//Saturate the velocity if it is too high
//There is a problem with this and I am not sure what yet (Bohlsen)
void Leveller::ApplySaturationFilter() {
    if(actuator_velocity_target_.x > saturation_velocity_) {actuator_velocity_target_.x = saturation_velocity_; printf("Saturated 0\n");}
    else if(actuator_velocity_target_.x < -saturation_velocity_) {actuator_velocity_target_.x = -saturation_velocity_; printf("Saturated 0\n");}

    if(actuator_velocity_target_.y > saturation_velocity_) {actuator_velocity_target_.y = saturation_velocity_; printf("Saturated 1\n");}
    else if(actuator_velocity_target_.y < -saturation_velocity_) {actuator_velocity_target_.y = -saturation_velocity_; printf("Saturated 1\n");}

    if(actuator_velocity_target_.z > saturation_velocity_) {actuator_velocity_target_.z = saturation_velocity_;printf("Saturated 2\n");}
    else if(actuator_velocity_target_.z < -saturation_velocity_) {actuator_velocity_target_.z = -saturation_velocity_; printf("Saturated 2\n");}
}

