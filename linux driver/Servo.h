//Servo.h
#pragma once
#define PI 3.14159265

#include <gsl/gsl_blas.h>
// Linux headers
#include <unistd.h> // write(), read(), close()
#include <iostream>
#include <fstream>
#include <string>


namespace Servo 
{
    const double distance_per_microstep_motor = 0.000000297;
    const double distance_per_microstep_actuator = 0.000004/256.0; //THIS IS A PLACEHOLDER VALUE UNTIL I CHECK THE REAL ONE

    struct Doubles 
    {
        double x = 0; 
        double y = 0;
        double z = 0;
    };

    struct DoublesSixAxis
    {
        double x = 0; 
        double y = 0;
        double z = 0;
        double r = 0; //Roll
        double p = 0; //Pitch
        double s = 0; //Yaw
    };

    struct Steps
    {
        int motor_0 = 0;
        int motor_1 = 0;
        int motor_2 = 0;
    };

    class Leveller 
    {
        public:
            Doubles actuator_velocity_target_;
            Doubles last_actuator_velocity_target_;
            Doubles acc0_latest_measurements_;
            Doubles acc1_latest_measurements_;
            Doubles acc2_latest_measurements_;
            Doubles acc_estimate_; 
            Steps current_step_count_; //These are actually microstep counts
            void UpdateTarget();

            double pitch_estimate_arr_ [10] = {0}; //We initialise pitch and roll estimates to zero
            double roll_estimate_arr_ [10] = {0};
            double pitch_estimate_filtered_ = 0;
            double roll_estimate_filtered_ = 0;

            bool enable_flag_ = false; //A flag to track whether the leveller should run or not

        private:  
            static double saturation_velocity_;

            void CombineAccelerations();
            void EstimateState();
            void ApplyLQRGain();
            void ApplySaturationFilter();
    };

    class Navigator
    {
        public:
            Doubles motor_velocity_target_;
            Steps current_step_count_; //These are actually microstep counts
            void UpdateTarget();
            void SetNewPosition(Doubles new_position);

            bool enable_flag_ = false;

        private:
            Doubles position_target_;
            Doubles BFF_velocity_target_;
            Doubles distance_from_reference_; //Note that this records the x,y, and yaw displacements
            static double gain_x_;
            static double gain_y_;
            static double gain_yaw_;
            static double saturation_velocity_;

            void ComputeState();
            void ApplyPropGain();
            void ApplySaturationFilter();
    };

    class Stabiliser 
    {
        public:
            Stabiliser();
            //Targets for the state elements we aim to control
            Doubles BFF_vel_reference_;
            Doubles BFF_pos_reference_; //NOTE: This will just be the integral of the above
            Doubles angle_reference_;

            //Structures to store the measured values
            Doubles acc0_latest_measurements_;
            Doubles acc1_latest_measurements_;
            Doubles acc2_latest_measurements_;
            Doubles acc3_latest_measurements_;
            Doubles acc4_latest_measurements_;
            Doubles acc5_latest_measurements_;
            DoublesSixAxis platform_position_measurement_;
            Steps motor_steps_measurement_;
            Steps actuator_steps_measurement_;

            //Defining the input variables
            Doubles motor_velocity_target_;
            Doubles actuator_velocity_target_;
            
            void UpdateTarget();

            //Structures to store the state estimate
            DoublesSixAxis opto_pos_estimate_;
            DoublesSixAxis opto_vel_estimate_;
            DoublesSixAxis plat_pos_estimate_;
            DoublesSixAxis plat_vel_estimate_;
            DoublesSixAxis plat_acc_estimate_;

            
            //Structures to cache the previous state estimate
            DoublesSixAxis opto_pos_estimate_prior_;
            DoublesSixAxis opto_vel_estimate_prior_;
            DoublesSixAxis plat_pos_estimate_prior_;
            DoublesSixAxis plat_vel_estimate_prior_;
            DoublesSixAxis plat_acc_estimate_prior_;

            //Structure to store the velocity we want to apply to the platform
            DoublesSixAxis input_velocity_;

            //Structure to store the computed perturbation to the velocity
            DoublesSixAxis plat_vel_perturbation_;

            bool enable_flag_ = false; //A flag to track whether the leveller should run or not

        private:  
            static double motor_saturation_velocity_;
            static double actuator_saturation_velocity_;
            static double dt_; //Time period for a controller loop (will be assumed to be 0.001)

            double F_ [30*30]; //Array to store the values of the state transition matrix 
            double H_ [24*30]; //Array to store the values of the measurement matrix 
            double B_ [30*6]; //Array to store the values of the input-state coupling matrix 
            double G_ [30*24]; //Array to store the values of the Kalman Gain matrix
            double K_ [6*30]; //Array to store the proportional LQR gain

            //array to store the estimated state vector
            double x_hat_ [30];
            //array to store the prior estimated state vector
            double x_hat_prior_ [30];
            //Defining the array of output variables
            //Indices 0-17 list the accelerometer readings in order
            //Indices 18-24 list the rough measurement of platform position
            double y_ [24];
            //array to store the requested perturbations to the platform velocity
            double u_ [6];

            //Buffer vectors to store results of sub-computations necessary to the 
            //Kalman updating procedure
            gsl_vector *x_hat_state_ptr_ = gsl_vector_alloc(30);
            gsl_vector *x_hat_input_ptr_ = gsl_vector_alloc(30);
            gsl_vector *x_hat_kalman_ptr_ = gsl_vector_alloc(30);

            void ConstructMatrices(); //Reads in the gain matrices from external files. Runs at construction

            //These just pass values from the user readable structs above into arrays that BLAS can handle
            void ConstructStateEstimateArray();
            void ConstructOutputArray();
            void ConstructInputArray();

            //Passes the arrays created by the above back into user readable structs
            void DeconstructStateEstimateArray();
            void DeconstructInputArray();

            void RunLogicalDistanceSensor();
            void EstimateStateAndApplyGain();
            void ApplyLQRGain();
            void ConvertToMotorVelocity();
            void ApplySaturationFilter();
    };
}