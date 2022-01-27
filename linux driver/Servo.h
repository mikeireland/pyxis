//Servo.h
#pragma once
#define PI 3.14159265

namespace Servo 
{
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
            //Targets for the state elements we aim to control
            Doubles BFF_vel_reference_;
            Doubles angle_reference_;

            //Defining the output variables
            Doubles acc0_latest_measurements_;
            Doubles acc1_latest_measurements_;
            Doubles acc2_latest_measurements_;
            Doubles acc3_latest_measurements_;
            Doubles acc4_latest_measurements_;
            Doubles acc5_latest_measurements_;

            //It is convenient to store the following
            DoublesSixAxis input_velocity_;

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

            bool enable_flag_ = false; //A flag to track whether the leveller should run or not

        private:  
            static double motor_saturation_velocity_;
            static double actuator_saturation_velocity_;

            void EstimateState();
            void ApplyLQRGain();
            void ConvertToMotorVelocity();
            void ApplySaturationFilter();
    };
}