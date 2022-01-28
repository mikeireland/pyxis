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
    for(int i = 1; i <= 9; i++) {
        pitch_estimate_arr_[i-1] = pitch_estimate_arr_[i];
        roll_estimate_arr_[i-1] = roll_estimate_arr_[i];
    }

    pitch_estimate_arr_[9] =  atan(-acc_estimate_.x/sqrt(acc_estimate_.z*acc_estimate_.z+
                                                  acc_estimate_.y*acc_estimate_.y))*180/PI;
    roll_estimate_arr_[9] = -atan(acc_estimate_.y/acc_estimate_.z)*180/PI;

    pitch_estimate_filtered_ = 0.0;
    roll_estimate_filtered_ = 0.0;

    for(int i = 0; i <= 9; i++) {
        pitch_estimate_filtered_ += pitch_estimate_arr_[i]/10.0;
        roll_estimate_filtered_ += roll_estimate_arr_[i]/10.0;
    }

    //printf("Pitch is %f\n",pitch_estimate_filtered_);
    //printf("Roll is %f\n",roll_estimate_filtered_);
}

void Leveller::ApplyLQRGain() {
    actuator_velocity_target_.x = -0.0001*(4*pitch_estimate_filtered_-7*roll_estimate_filtered_);
    actuator_velocity_target_.y = -0.0001*(-8*pitch_estimate_filtered_);
    actuator_velocity_target_.z = -0.0001*(4*pitch_estimate_filtered_+7*roll_estimate_filtered_);
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

/*
DEFINITIONS FOR THE Stabiliser OBJECT
*/

//Set a saturation velocity of 1000 micron/s
double Stabiliser::motor_saturation_velocity_ = 0.002;
double Stabiliser::actuator_saturation_velocity_ = 0.001;
double Stabiliser::dt_ = 0.001;

//Function to update Leveller's target velocitites
//TODO Add a Kalman filter to better estimate the state
void Stabiliser::UpdateTarget() {

    //cacher the previous state estimate for use later
    opto_pos_estimate_prior_ = opto_pos_estimate_;
    opto_vel_estimate_prior_ = opto_vel_estimate_;
    plat_pos_estimate_prior_ = plat_pos_estimate_;
    plat_vel_estimate_prior_ = plat_vel_estimate_;
    plat_acc_estimate_prior_ = plat_acc_estimate_; 

    EstimateState();
    ApplyLQRGain();
    ConvertToMotorVelocity();
    ApplySaturationFilter();
}

//Compute the Kalman filter predicted state for the system
void Stabiliser::EstimateState() {
}

//Compute the LQR suggested physical velocity inputs
void Stabiliser::ApplyLQRGain() {
    input_velocity_.x = BFF_vel_reference_.x;
    input_velocity_.y = BFF_vel_reference_.y;
    input_velocity_.s = BFF_vel_reference_.z;
}

//Transform from physical velocties back into the velocities of the motors
void Stabiliser::ConvertToMotorVelocity() {
    motor_velocity_target_.x = -input_velocity_.y - input_velocity_.s;
    motor_velocity_target_.y = ( input_velocity_.x * sin(PI / 3) + input_velocity_.y * cos(PI / 3)) - input_velocity_.s;
    motor_velocity_target_.z = (-input_velocity_.x * sin(PI / 3) + input_velocity_.y * cos(PI / 3)) - input_velocity_.s;

    actuator_velocity_target_.x = 0;
    actuator_velocity_target_.y = 0;
    actuator_velocity_target_.z = 0;
}

//Saturate the velocity if it is too high
void Stabiliser::ApplySaturationFilter() {
}

void Stabiliser::BLASTest() {
    //Defining the data for the arrays
    
    ConstructMatrices();

     double C_ [4] = {0.0,0.0,
                    0.0,0.0};
    
    //Converting the arrays to gsl_matrices (BLAS wrapper)
    gsl_matrix_view G_matrix_ = gsl_matrix_view_array(G_,2,3);
    gsl_matrix_view H_matrix_ = gsl_matrix_view_array(H_,3,2);
    gsl_matrix_view C_matrix_ = gsl_matrix_view_array(C_,2,2);
    //Compute C = GH
    gsl_blas_dgemm(CblasNoTrans,CblasNoTrans,
                    1.0, &G_matrix_.matrix, &H_matrix_.matrix,
                    0.0, &C_matrix_.matrix);
         
    printf ("[ %g, %g\n", C_[0], C_[1]);
    printf ("  %g, %g ]\n", C_[2], C_[3]);    
}


void Stabiliser::ConstructMatrices() {
    G_ [0] = 1.0;
    G_ [1] = 2.0;
    G_ [2] = 3.0;

    G_ [3] = 4.0;
    G_ [4] = 5.0;
    G_ [5] = 6.0;
 
    H_ [0] = 10.0;
    H_ [1] = 20.0;
    H_ [2] = 30.0;

    H_ [3] = 40.0;
    H_ [4] = 50.0;
    H_ [5] = 60.0;
}

void Stabiliser::ConstructStateEstimateArray() {
    for(int i = 0; i < 30; i++) {
        x_hat_[i] = 0;
    }

    x_hat_prior_[0] = opto_pos_estimate_prior_.x;
    x_hat_prior_[1] = opto_pos_estimate_prior_.y;
    x_hat_prior_[2] = opto_pos_estimate_prior_.z;
    x_hat_prior_[3] = opto_pos_estimate_prior_.r;
    x_hat_prior_[4] = opto_pos_estimate_prior_.p;
    x_hat_prior_[5] = opto_pos_estimate_prior_.s;
    x_hat_prior_[6] = opto_vel_estimate_prior_.x;
    x_hat_prior_[7] = opto_vel_estimate_prior_.y;
    x_hat_prior_[8] = opto_vel_estimate_prior_.z;
    x_hat_prior_[9] = opto_vel_estimate_prior_.r;
    x_hat_prior_[10] = opto_vel_estimate_prior_.p;
    x_hat_prior_[11] = opto_vel_estimate_prior_.s;
    x_hat_prior_[12] = plat_pos_estimate_prior_.x;
    x_hat_prior_[13] = plat_pos_estimate_prior_.y;
    x_hat_prior_[14] = plat_pos_estimate_prior_.z;
    x_hat_prior_[15] = plat_pos_estimate_prior_.r;
    x_hat_prior_[16] = plat_pos_estimate_prior_.p;
    x_hat_prior_[17] = plat_pos_estimate_prior_.s;
    x_hat_prior_[18] = plat_vel_estimate_prior_.x;
    x_hat_prior_[19] = plat_vel_estimate_prior_.y;
    x_hat_prior_[20] = plat_vel_estimate_prior_.z;
    x_hat_prior_[21] = plat_vel_estimate_prior_.r;
    x_hat_prior_[22] = plat_vel_estimate_prior_.p;
    x_hat_prior_[23] = plat_vel_estimate_prior_.s;
    x_hat_prior_[24] = plat_acc_estimate_prior_.x;
    x_hat_prior_[25] = plat_acc_estimate_prior_.y;
    x_hat_prior_[26] = plat_acc_estimate_prior_.z;
    x_hat_prior_[27] = plat_acc_estimate_prior_.r;
    x_hat_prior_[28] = plat_acc_estimate_prior_.p;
    x_hat_prior_[29] = plat_acc_estimate_prior_.s;
}

void Stabiliser::ConstructOutputArray() {
    //Writing in the last accelerometer measurements
    y_[0] = acc0_latest_measurements_.x;
    y_[1] = acc0_latest_measurements_.y;
    y_[2] = acc0_latest_measurements_.z;
    y_[3] = acc1_latest_measurements_.x;
    y_[4] = acc1_latest_measurements_.y;
    y_[5] = acc1_latest_measurements_.z;
    y_[6] = acc2_latest_measurements_.x;
    y_[7] = acc2_latest_measurements_.y;
    y_[8] = acc2_latest_measurements_.z;
    y_[9] = acc3_latest_measurements_.x;
    y_[10] = acc3_latest_measurements_.y;
    y_[11] = acc3_latest_measurements_.z;
    y_[12] = acc4_latest_measurements_.x;
    y_[13] = acc4_latest_measurements_.y;
    y_[14] = acc4_latest_measurements_.z;
    y_[15] = acc5_latest_measurements_.x;
    y_[16] = acc5_latest_measurements_.y;
    y_[17] = acc5_latest_measurements_.z;

    //Writing in the current approximation of the platform position
    y_[18] = platform_position_measurement_.x;
    y_[19] = platform_position_measurement_.y;
    y_[20] = platform_position_measurement_.z;
    y_[21] = platform_position_measurement_.r;
    y_[22] = platform_position_measurement_.p;
    y_[23] = platform_position_measurement_.s;
}

//This subroutine isn't strictly necessary but is included for symmetry
void Stabiliser::ConstructInputArray() {
    for(int i = 0; i < 6; i++) {
        u_[i] = 0;
    }
}
