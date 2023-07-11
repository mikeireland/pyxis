//Servo.cpp
#include "Servo.h"
#include <math.h>
#include <stdio.h>

using namespace Servo;

/*
LEVELLER DEFINITIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    acc_estimate_.z = (acc0_latest_measurements_.z
                       +acc1_latest_measurements_.z
                       +acc2_latest_measurements_.z)/3.0;
}

//Compute the pitch and roll given the value of the accelerations
void Leveller::EstimateState() {
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

    printf("Pitch is %f\n",pitch_estimate_filtered_);
    printf("Roll is %f\n",roll_estimate_filtered_);

    pitch_estimate_filtered_ = pitch_estimate_filtered_ - pitch_target_;
    roll_estimate_filtered_ = roll_estimate_filtered_ - roll_target_;

   
}

void Leveller::ApplyLQRGain() {
    actuator_velocity_target_.x = -0.0001*(-4*pitch_estimate_filtered_+ 7*roll_estimate_filtered_);
    actuator_velocity_target_.y = -0.0001*(8*pitch_estimate_filtered_);
    actuator_velocity_target_.z = -0.0001*(-4*pitch_estimate_filtered_- 7*roll_estimate_filtered_);
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
NAVIGATOR DEFINITIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    double temp_x   =                                     -0.5774*current_step_count_.motor_1+0.5774*current_step_count_.motor_2;//x displacement
    double temp_y   = -0.6667*current_step_count_.motor_0+0.3333*current_step_count_.motor_1+0.3333*current_step_count_.motor_2;//y displacement
    double temp_yaw = -0.3333*current_step_count_.motor_0-0.3333*current_step_count_.motor_1-0.3333*current_step_count_.motor_2;//yaw displacement

    //Note that the above are being stored in units of microsteps. To convert to metres we note that there are 295nm in a microstep 
    //of the motors and therefore that we have
    distance_from_reference_.x = temp_x*distance_per_microstep_motor;
    distance_from_reference_.y = temp_y*distance_per_microstep_motor;

    //For the yaw we must also multiply by the rotational factor to recognise that our yaw is a dimensionless quantity in radians
    //Note that the 0.3 below represents a 30cm radius for the rotation. This is just a ballpark figure and will need to 
    //be made more accurate for any precision purposes
    distance_from_reference_.z = temp_yaw*(distance_per_microstep_motor/0.3);

    /* MUST CHANGE THE ABOVE AND THE SAME IN THE STABILISER DO NOT FORGET !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    */
}

void Navigator::ApplyPropGain() {
    //We apply a proportional gain to compute target Body Fixed Frame velocities
    BFF_velocity_target_.x = -gain_x_*(distance_from_reference_.x-position_target_.x);
    BFF_velocity_target_.y = -gain_y_*(distance_from_reference_.y-position_target_.y);
    BFF_velocity_target_.z = -gain_yaw_*(distance_from_reference_.z-position_target_.z);

    //We then convert from the target BFF velocitities to the 
    //read velocity of each of the motors 
    motor_velocity_target_.x = -BFF_velocity_target_.y - BFF_velocity_target_.z;
    motor_velocity_target_.y = (-BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    motor_velocity_target_.z = (BFF_velocity_target_.x * sin(PI / 3) + BFF_velocity_target_.y * cos(PI / 3)) - BFF_velocity_target_.z;
    //printf("%f,%f,%f\n",motor_velocity_target_.x,motor_velocity_target_.y,motor_velocity_target_.z);

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
STABILISER DEFINITIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

//Set a saturation velocity of 1000 micron/s
double Stabiliser::motor_saturation_velocity_ = 0.015;
double Stabiliser::actuator_saturation_velocity_ = 0.0005;
double Stabiliser::dt_ = 0.001;
double Stabiliser::global_gain_ = 0.05;


Stabiliser::Stabiliser() {
    //We initialise the matrices for the gains at class construction
    //This ensures that they will always exist
    ConstructMatrices();
    ConstructStateEstimateArray();
    ConstructStateReferenceArray();
    ConstructInputArray();
    ConstructOutputArray();
}

void Stabiliser::UpdateTarget() {

    //cache the previous state estimate for use later
    opto_pos_estimate_prior_ = opto_pos_estimate_;
    opto_vel_estimate_prior_ = opto_vel_estimate_;
    plat_pos_estimate_prior_ = plat_pos_estimate_;
    plat_vel_estimate_prior_ = plat_vel_estimate_;
    plat_acc_estimate_prior_ = plat_acc_estimate_; 

    time_ += 1;

    RunLogicalDistanceSensor();
    EstimateStateAndApplyGain();
    ConvertToMotorVelocity();
    ApplySaturationFilter();
}

//For the Kalman filter to work well a logical distance sensor is required which creates an inaccurate measurement
//of the platform position from the step counts
void Stabiliser::RunLogicalDistanceSensor() {

    //We first push the motor step count into its associated buffer with 
    platform_position_measurement_.x = distance_per_microstep_motor*(-0.5774*motor_steps_measurement_.motor_1+0.5774*motor_steps_measurement_.motor_2);//x displacement
    platform_position_measurement_.y = distance_per_microstep_motor*(-0.6667*motor_steps_measurement_.motor_0+0.3333*motor_steps_measurement_.motor_1+0.3333*motor_steps_measurement_.motor_2);//y displacement
    platform_position_measurement_.s = (distance_per_microstep_motor/0.3)*(-0.3333*motor_steps_measurement_.motor_0-0.3333*motor_steps_measurement_.motor_1-0.3333*motor_steps_measurement_.motor_2);//yaw displacement

    //We then push the actuator step counts into their buffers
    //Remember that an actuator step refers to a downwards motion
    platform_position_measurement_.z = -distance_per_microstep_actuator*0.3333*(actuator_steps_measurement_.motor_0+actuator_steps_measurement_.motor_1+actuator_steps_measurement_.motor_2);
    platform_position_measurement_.r = -(PI/180)*distance_per_microstep_actuator*(194.0*actuator_steps_measurement_.motor_0-194.0*actuator_steps_measurement_.motor_2);
    platform_position_measurement_.p = -(PI/180)*distance_per_microstep_actuator*(-112.0*actuator_steps_measurement_.motor_0+224.0*actuator_steps_measurement_.motor_1-112.0*actuator_steps_measurement_.motor_2);
}

//Compute the Kalman filter predicted state for the system
void Stabiliser::EstimateStateAndApplyGain() {

    //printf("\n\n\n\n\n"); 

    //We update the arrays to store state variables
    ConstructStateEstimateArray();
    ConstructStateReferenceArray();
    ConstructInputArray();
    ConstructOutputArray();

    //We push the main model vectors to gsl vectors
    gsl_vector_view x_hat_vector = gsl_vector_view_array(x_hat_,30);
    gsl_vector_view x_hat_prior_vector = gsl_vector_view_array(x_hat_prior_,30);
    gsl_vector_view r_vector = gsl_vector_view_array(r_,30);
    gsl_vector_view y_vector = gsl_vector_view_array(y_,30);
    gsl_vector_view u_vector = gsl_vector_view_array(u_,6);

    //We push the gain matrices to gsl matrices
    gsl_matrix_view F_matrix = gsl_matrix_view_array(F_,30,30);
    gsl_matrix_view B_matrix = gsl_matrix_view_array(B_,30,6);
    gsl_matrix_view H_matrix = gsl_matrix_view_array(H_,30,30);
    gsl_matrix_view G_matrix = gsl_matrix_view_array(G_,30,30);
    gsl_matrix_view K_matrix = gsl_matrix_view_array(K_,6,30);

    //Compute the expected output from the prior state
    gsl_blas_dgemv(CblasNoTrans,
                    -1.0,&H_matrix.matrix,&x_hat_prior_vector.vector,
                    1.0,&y_vector.vector); //We are computing (1.0*y-1.0*H.x_at) and we store it in the y_vector for convenience
    //NOTE: y_vector is now storing y_error = y-H.x_hat. 

    //Compute the noise influence on the new state and pass it to the new x_hat_kalman_ buffer vector
    gsl_blas_dgemv(CblasNoTrans,
                    1.0, &G_matrix.matrix, &y_vector.vector,
                    0.0, x_hat_kalman_ptr_);

    //Compute the input influence on the new state estimate
    gsl_blas_dgemv(CblasNoTrans,
                    1.0, &B_matrix.matrix, &u_vector.vector,
                    0.0, x_hat_input_ptr_);

    //Compute the previous state estimate's influence on the new state estimate
    gsl_blas_dgemv(CblasNoTrans,
                    1.0, &F_matrix.matrix, &x_hat_prior_vector.vector,
                    0.0, x_hat_state_ptr_);

    //Now, we sum up all three of the vectors x_hat_vector_
    gsl_blas_daxpy(1.0,x_hat_kalman_ptr_,&x_hat_vector.vector);
    gsl_blas_daxpy(1.0,x_hat_input_ptr_,&x_hat_vector.vector);
    gsl_blas_daxpy(1.0,x_hat_state_ptr_,&x_hat_vector.vector);

    //To ensure we cache the whole state for the next loop we Deconstruct before removing the reference
    DeconstructStateEstimateArray();
    /*
    for(int i = 0; i < 30; i++){
        printf("%f,%f,%f\n",x_hat_[i],r_[i],x_hat_[i]-r_[i]);
    }
    */

    //For tracking purposes we remove the reference from the 
    //state and store the new error state in a new state vector
    gsl_blas_daxpy(-1.0,&r_vector.vector,&x_hat_vector.vector);
   

    /*
    The x_hat_ array now represents a prediction for the state of the system at the next timestep.
    We then take that prediction and compute what feedback input we should apply (note the -1.0 for negative feedback)
    */

   gsl_blas_dgemv(CblasNoTrans,
                   -1.0, &K_matrix.matrix, &x_hat_vector.vector,
                   0.0, &u_vector.vector);

    //We pass newly computed values back into the controllers public buffers
    DeconstructInputArray();
}

//Transform from physical velocties back into the velocities of the motors
void Stabiliser::ConvertToMotorVelocity() {
    //We turn the perturbations into the new velocity values we will apply
    
    input_velocity_.x = input_velocity_.x+dt_*plat_vel_perturbation_.x;
    input_velocity_.y = input_velocity_.y+dt_*plat_vel_perturbation_.y;
    input_velocity_.z = input_velocity_.z+dt_*plat_vel_perturbation_.z;
    input_velocity_.r = input_velocity_.r+dt_*plat_vel_perturbation_.r;
    input_velocity_.p = input_velocity_.p+dt_*plat_vel_perturbation_.p;
    input_velocity_.s = input_velocity_.s+dt_*plat_vel_perturbation_.s;

    platform_velocity_measurement_.x = input_velocity_.x;
    platform_velocity_measurement_.y = input_velocity_.y;
    platform_velocity_measurement_.z = input_velocity_.z;
    platform_velocity_measurement_.r = input_velocity_.r;
    platform_velocity_measurement_.p = input_velocity_.p;
    platform_velocity_measurement_.s = input_velocity_.s;

    /*
    if(input_velocity_.x > motor_saturation_velocity_) {input_velocity_.x = motor_saturation_velocity_; printf("Saturated x\n");}
    else if(input_velocity_.x < -motor_saturation_velocity_) {input_velocity_.x = -motor_saturation_velocity_; printf("Saturated x\n");}

    if(input_velocity_.y > motor_saturation_velocity_) {input_velocity_.y = motor_saturation_velocity_; printf("Saturated y\n");}
    else if(input_velocity_.y < -motor_saturation_velocity_) {input_velocity_.y = -motor_saturation_velocity_; printf("Saturated y\n");}

    if(input_velocity_.r > actuator_saturation_velocity_) {input_velocity_.r = actuator_saturation_velocity_; printf("Saturated Roll\n");}
    else if(input_velocity_.r < -actuator_saturation_velocity_) {input_velocity_.r = -actuator_saturation_velocity_; printf("Saturated Roll\n");}

    if(input_velocity_.p > actuator_saturation_velocity_) {input_velocity_.p = actuator_saturation_velocity_; printf("Saturated Pitch\n");}
    else if(input_velocity_.p < -actuator_saturation_velocity_) {input_velocity_.p = -actuator_saturation_velocity_; printf("Saturated Pitch\n");}

    if(input_velocity_.z > actuator_saturation_velocity_) {input_velocity_.z = actuator_saturation_velocity_;printf("Saturated z\n");}
    else if(input_velocity_.z < -actuator_saturation_velocity_) {input_velocity_.z = -actuator_saturation_velocity_; printf("Saturated z\n");}

    if(input_velocity_.s > motor_saturation_velocity_) {input_velocity_.s = motor_saturation_velocity_;printf("Saturated s\n");}
    else if(input_velocity_.s < -motor_saturation_velocity_) {input_velocity_.s = -motor_saturation_velocity_; printf("Saturated s\n");}    
    */

    /*
    printf("Input Velocity x = %f,Perturbation = %f\n",input_velocity_.x,plat_vel_perturbation_.x);
    printf("Input Velocity y = %f, Perturbation = %f\n",input_velocity_.y,plat_vel_perturbation_.y);
    printf("Input Velocity z = %f, Perturbation = %f\n",input_velocity_.z,plat_vel_perturbation_.z);
    printf("Input Velocity r = %f, Perturbation = %f\n",input_velocity_.r,plat_vel_perturbation_.r);
    printf("Input Velocity p = %f, Perturbation = %f\n",input_velocity_.p,plat_vel_perturbation_.p);
    printf("Input Velocity s = %f, Perturbation = %f\n",input_velocity_.s,plat_vel_perturbation_.s);
    */

    //For details on these transformations see the Stabiliser Transformations Mathematica notebook or the full Stabiliser documentation 
    //Note that the transformations to degrees adjust for the fact that the Stabiliser Transformations notebooks works with pith and roll in degrees
    //This was a convenience and could be changed with no consequence
    motor_velocity_target_.x = -input_velocity_.y - input_velocity_.s;                                                     //Motor 0
    motor_velocity_target_.y = ( - input_velocity_.x * sin(PI / 3) + input_velocity_.y * cos(PI / 3)) - input_velocity_.s;   //Motor 1
    motor_velocity_target_.z = (input_velocity_.x * sin(PI / 3) + input_velocity_.y * cos(PI / 3)) - input_velocity_.s;   //Motor 2
    actuator_velocity_target_.x = input_velocity_.z + (180.0/PI)*((1.0/388.0)*input_velocity_.r - (1.0/672.0)*input_velocity_.p); //Actuator 0
    actuator_velocity_target_.y = input_velocity_.z + (1.0/336.0)*(180.0/PI)*input_velocity_.p;                                  //Actuator 1
    actuator_velocity_target_.z = input_velocity_.z + (180.0/PI)*(- (1.0/388.0)*input_velocity_.r - (1.0/672.0)*input_velocity_.p); //Actuator 2
}

//Saturate the velocity if it is too high
void Stabiliser::ApplySaturationFilter() {
    
    if(motor_velocity_target_.x > motor_saturation_velocity_) {motor_velocity_target_.x = motor_saturation_velocity_; printf("Saturated Motor 0\n");}
    else if(motor_velocity_target_.x < -motor_saturation_velocity_) {motor_velocity_target_.x = -motor_saturation_velocity_; printf("Saturated Motor 0\n");}

    if(motor_velocity_target_.y > motor_saturation_velocity_) {motor_velocity_target_.y = motor_saturation_velocity_; printf("Saturated Motor 1\n");}
    else if(motor_velocity_target_.y < -motor_saturation_velocity_) {motor_velocity_target_.y = -motor_saturation_velocity_; printf("Saturated Motor 1\n");}

    if(motor_velocity_target_.z > motor_saturation_velocity_) {motor_velocity_target_.z = motor_saturation_velocity_;printf("Saturated Motor 2\n");}
    else if(motor_velocity_target_.z < -motor_saturation_velocity_) {motor_velocity_target_.z = -motor_saturation_velocity_; printf("Saturated Motor 2\n");}

    if(actuator_velocity_target_.x > actuator_saturation_velocity_) {actuator_velocity_target_.x = actuator_saturation_velocity_; printf("Saturated Actuator 0\n");}
    else if(actuator_velocity_target_.x < -actuator_saturation_velocity_) {actuator_velocity_target_.x = -actuator_saturation_velocity_; printf("Saturated Actuator 0\n");}

    if(actuator_velocity_target_.y > actuator_saturation_velocity_) {actuator_velocity_target_.y = actuator_saturation_velocity_; printf("Saturated Actuator 1\n");}
    else if(actuator_velocity_target_.y < -actuator_saturation_velocity_) {actuator_velocity_target_.y = -actuator_saturation_velocity_; printf("Saturated Actuator 1\n");}

    if(actuator_velocity_target_.z > actuator_saturation_velocity_) {actuator_velocity_target_.z = actuator_saturation_velocity_;printf("Saturated Actuator 2\n");}
    else if(actuator_velocity_target_.z < -actuator_saturation_velocity_) {actuator_velocity_target_.z = -actuator_saturation_velocity_; printf("Saturated Actuator 2\n");}
    
}

void Stabiliser::ConstructMatrices() {
    //NOTE THAT THE MATRICES BELOW ARE NOT NECESSARILY IDENTICAL TO THE MATRICES USED TO COMPUTE G AND K
    //THEY ARE MINOR VARIATIONS CHOSEN TO INTRODUCE MINIMAL ERROR WHILE IMPROVING PERFORMANCE AND COMPUTATION SAFETY
    /*
    Reading in the state transition matrix
    */
   std::ifstream file;
   file.open("StabiliserMatrices/FDriverStabiliser.txt",std::ios::in);
   std::string string_buffer = "";
   int value_count = 0;
   while(value_count < 30*30) {
    char char_in = file.get();
    if((char_in == ',') | (char_in == '\n') ) {
        F_[value_count] = std::stod(string_buffer);
        string_buffer = "";
        value_count += 1;
    }
    else {
        string_buffer += char_in;
    }
   }
   file.close();

   /*
    Reading in the input-state coupling matrix
    */
   file.open("StabiliserMatrices/BDriverStabiliser.txt",std::ios::in);
   string_buffer = "";
   value_count = 0;
   while(value_count < 30*6) {
    char char_in = file.get();
    if((char_in == ',') | (char_in == '\n') ) {
        B_[value_count] = std::stod(string_buffer);
        string_buffer = "";
        value_count += 1;
    }
    else {
        string_buffer += char_in;
    }
   }
   file.close();

   /*
    Reading in the measurement matrix
    */
   file.open("StabiliserMatrices/HDriverStabiliser.txt",std::ios::in);
   string_buffer = "";
   value_count = 0;
   while(value_count < 20*30) {
    char char_in = file.get();
    if((char_in == ',') | (char_in == '\n') ) {
        H_[value_count] = std::stod(string_buffer);
        string_buffer = "";
        value_count += 1;
    }
    else {
        string_buffer += char_in;
    }
   }
   file.close();

   /*
    Reading in the Kalman Gain matrix
    */
   file.open("StabiliserMatrices/GDriverStabiliser.txt",std::ios::in);
   string_buffer = "";
   value_count = 0;
   while(value_count < 30*30) {
    char char_in = file.get();
    if((char_in == ',') | (char_in == '\n') ) {
        G_[value_count] = std::stod(string_buffer);
        string_buffer = "";
        value_count += 1;
    }
    else {
        string_buffer += char_in;
    }
   }
   file.close();

   /*
    Reading in the LQR Gain matrix
    */
   file.open("StabiliserMatrices/KDriverStabiliser.txt",std::ios::in);
   string_buffer = "";
   value_count = 0;
   while(value_count < 6*30) {
    char char_in = file.get();
    if((char_in == ',') | (char_in == '\n') ) {
        K_[value_count] = std::stod(string_buffer);
        string_buffer = "";
        value_count += 1;
    }
    else {
        string_buffer += char_in;
    }
   }
   file.close();

   /*

    //Print the gain matrices as the end as a check
   for(int i = 0; i < 30*30; i++) {
       printf("%f,",F_[i]);
       if(i%30 == 29){
           printf("\n");
       }
   }
   printf("\n\n\n\n\n");
   for(int i = 0; i < 30*6; i++) {
       printf("%f,",B_[i]);
       if(i%6 == 5){
           printf("\n");
       }
   }
   printf("\n\n\n\n\n");
   for(int i = 0; i < 30*30; i++) {
       printf("%f,",H_[i]);
       if(i%30 == 29){
           printf("\n");
       }
   }
   printf("\n\n\n\n\n");
   for(int i = 0; i < 30*30; i++) {
       printf("%f,",G_[i]);
       if(i%30 == 29){
           printf("\n");
       }
   }
   printf("\n\n\n\n\n");
   for(int i = 0; i < 6*30; i++) {
       printf("%f,",K_[i]);
       if(i%30 == 29){
           printf("\n");
       }
   }
   */
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

void Stabiliser::ConstructStateReferenceArray() {
    //We construct the postion reference as the time integral of the 
    //velocity reference
    r_ [0] = BFF_vel_reference_.x*time_*dt_;
    r_ [1] = BFF_vel_reference_.y*time_*dt_;

    //Reference height is equilibrium
    r_ [2] = height_target_;

    r_ [3] = angle_reference_.x; //roll
    r_ [4] = angle_reference_.y; //pitch
    r_ [5] = BFF_vel_reference_.z*time_*dt_; //We integrate for yaw

    //target velocity
    r_ [6] = BFF_vel_reference_.x;
    r_ [7] = BFF_vel_reference_.y;
    r_ [8] = 0;

    //target angular velocity
    r_ [9] = 0;
    r_ [10] = 0;
    r_ [11] = BFF_vel_reference_.z;

    //Platform references
    r_ [12] = BFF_vel_reference_.x*time_*dt_;
    r_ [13] = BFF_vel_reference_.y*time_*dt_;
    r_ [14] = height_target_;
    r_ [15] = angle_reference_.x; 
    r_ [16] = angle_reference_.y; 
    r_ [17] = BFF_vel_reference_.z*time_*dt_;
    r_ [18] = BFF_vel_reference_.x;
    r_ [19] = BFF_vel_reference_.y;
    r_ [20] = 0;
    r_ [21] = 0;
    r_ [22] = 0;
    r_ [23] = BFF_vel_reference_.z;

    //All zeroed acceleration targets
    for(int i = 24; i < 30; i++) {
        r_ [i] = 0;
    }
}

void Stabiliser::ConstructOutputArray() {
    //Writing in the last accelerometer measurements and regularising about the target state
    //Not sure if this is the right way to do this yet, (N.B)
    y_[0] = acc0_latest_measurements_.x - acc0_ground_state_measurement_.x;
    y_[1] = acc0_latest_measurements_.y - acc0_ground_state_measurement_.y;
    y_[2] = acc0_latest_measurements_.z - acc0_ground_state_measurement_.z;
    y_[3] = acc1_latest_measurements_.x - acc1_ground_state_measurement_.x;
    y_[4] = acc1_latest_measurements_.y - acc1_ground_state_measurement_.y;
    y_[5] = acc1_latest_measurements_.z - acc1_ground_state_measurement_.z;
    y_[6] = acc2_latest_measurements_.x - acc2_ground_state_measurement_.x;
    y_[7] = acc2_latest_measurements_.y - acc2_ground_state_measurement_.y;
    y_[8] = acc2_latest_measurements_.z - acc2_ground_state_measurement_.z;
    y_[9] = acc3_latest_measurements_.x - acc3_ground_state_measurement_.x;
    y_[10] = acc3_latest_measurements_.y - acc3_ground_state_measurement_.y;
    y_[11] = acc3_latest_measurements_.z - acc3_ground_state_measurement_.z;
    y_[12] = acc4_latest_measurements_.x - acc4_ground_state_measurement_.x;
    y_[13] = acc4_latest_measurements_.y - acc4_ground_state_measurement_.y;
    y_[14] = acc4_latest_measurements_.z - acc4_ground_state_measurement_.z;
    y_[15] = acc5_latest_measurements_.x - acc5_ground_state_measurement_.x;
    y_[16] = acc5_latest_measurements_.y - acc5_ground_state_measurement_.y;
    y_[17] = acc5_latest_measurements_.z - acc5_ground_state_measurement_.z;

    //Writing in the current approximation of the platform position
    y_[18] = platform_position_measurement_.x;
    y_[19] = platform_position_measurement_.y;
    y_[20] = platform_position_measurement_.z;
    y_[21] = platform_position_measurement_.r;
    y_[22] = platform_position_measurement_.p;
    y_[23] = platform_position_measurement_.s;

    //Writing in the current approximation of the velocity position
    y_[24] = platform_velocity_measurement_.x;
    y_[25] = platform_velocity_measurement_.y;
    y_[26] = platform_velocity_measurement_.z;
    y_[27] = platform_velocity_measurement_.r;
    y_[28] = platform_velocity_measurement_.p;
    y_[29] = platform_velocity_measurement_.s;
}

//Writing the last requested perturbations to the input array
void Stabiliser::ConstructInputArray() {
    u_[0] = plat_vel_perturbation_.x;
    u_[1] = plat_vel_perturbation_.y;
    u_[2] = plat_vel_perturbation_.z;
    u_[3] = plat_vel_perturbation_.r;
    u_[4] = plat_vel_perturbation_.p;
    u_[5] = plat_vel_perturbation_.s;
}

void Stabiliser::DeconstructStateEstimateArray() {
    opto_pos_estimate_.x = x_hat_[0]; 
    opto_pos_estimate_.y = x_hat_[1]; 
    opto_pos_estimate_.z = x_hat_[2]; 
    opto_pos_estimate_.r = x_hat_[3]; 
    opto_pos_estimate_.p = x_hat_[4]; 
    opto_pos_estimate_.s = x_hat_[5]; 
    opto_vel_estimate_.x = x_hat_[6]; 
    opto_vel_estimate_.y = x_hat_[7]; 
    opto_vel_estimate_.z = x_hat_[8]; 
    opto_vel_estimate_.r = x_hat_[9]; 
    opto_vel_estimate_.p = x_hat_[10];
    opto_vel_estimate_.s = x_hat_[11];
    plat_pos_estimate_.x = x_hat_[12];
    plat_pos_estimate_.y = x_hat_[13];
    plat_pos_estimate_.z = x_hat_[14];
    plat_pos_estimate_.r = x_hat_[15];
    plat_pos_estimate_.p = x_hat_[16];
    plat_pos_estimate_.s = x_hat_[17];
    plat_vel_estimate_.x = x_hat_[18];
    plat_vel_estimate_.y = x_hat_[19];
    plat_vel_estimate_.z = x_hat_[20];
    plat_vel_estimate_.r = x_hat_[21];
    plat_vel_estimate_.p = x_hat_[22];
    plat_vel_estimate_.s = x_hat_[23];
    plat_acc_estimate_.x = x_hat_[24];
    plat_acc_estimate_.y = x_hat_[25];
    plat_acc_estimate_.z = x_hat_[26];
    plat_acc_estimate_.r = x_hat_[27];
    plat_acc_estimate_.p = x_hat_[28];
    plat_acc_estimate_.s = x_hat_[29];
}

void Stabiliser::DeconstructInputArray() {
    plat_vel_perturbation_.x = global_gain_*u_[0];
    plat_vel_perturbation_.y = 0.0*global_gain_*u_[1];
    plat_vel_perturbation_.z = 0.0*global_gain_*u_[2];
    plat_vel_perturbation_.r = 0.0*global_gain_*u_[3];
    plat_vel_perturbation_.p = 0.0*global_gain_*u_[4];
    plat_vel_perturbation_.s = 0.0*global_gain_*u_[5];
}
