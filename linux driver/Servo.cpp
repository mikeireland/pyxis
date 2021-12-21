//Servo.cpp
#include "Servo.h"
#include <math.h>
#include <stdio.h>

using namespace Servo;

//Function to update Leveller's target velocitites
//TODO Add a Kalman filter to better estimate the state
void Leveller::UpdateTarget() {
    CombineAccelerations();
    EstimateState();
    ApplyLQRGain();
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
    actuator_velocity_target_.x = 0.0001*(4*pitch_estimate_+7*roll_estimate_);
    actuator_velocity_target_.y = -0.0001*(-8*pitch_estimate_);
    actuator_velocity_target_.z = 0.0001*(4*pitch_estimate_-7*roll_estimate_);
}

