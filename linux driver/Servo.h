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

            double pitch_estimate_;
            double roll_estimate_;

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
}