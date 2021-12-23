//Servo.h
#pragma once
#define PI 3.14159265

namespace Servo 
{
    struct VelDoubles 
    {
        double x = 0; 
        double y = 0;
        double z = 0;
    };
    struct AccelDoubles
    {
        double x = 0; 
        double y = 0;
        double z = 0;
    };

    class Leveller 
    {
        public:
            VelDoubles actuator_velocity_target_;
            VelDoubles last_actuator_velocity_target_;
            AccelDoubles acc0_latest_measurements_;
            AccelDoubles acc1_latest_measurements_;
            AccelDoubles acc2_latest_measurements_;
            AccelDoubles acc_estimate_; 
            void UpdateTarget();

        //private:  //(I'm just making everythin public write now)
            double pitch_estimate_;
            double roll_estimate_;
            static double saturation_velocity_;

            void CombineAccelerations();
            void EstimateState();
            void ApplyLQRGain();
            void ApplySaturationFilter();
    };
}