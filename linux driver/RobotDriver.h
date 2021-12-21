//Controller.h
#pragma once
#include "SerialPort.h"


namespace Control 
{
    struct VelDoubles 
    {
        double x = 0; 
        double y = 0;
        double z = 0;
    };

    class RobotDriver
    {
        public:
            Comms::SerialPort teensy_port;

            VelDoubles BFF_velocity_target_; //We take these to be x,y,z = vx,vy,yaw 
            VelDoubles motor_velocities_target_; //For these take x,y,z == 0,1,2
            VelDoubles actuator_velocity_target_; //For these take x,y,z == 0,1,2
            void LinearSweep();
            void LinearLateralRamp(VelDoubles velocity_target,double time);
            void LinearYawRamp(double yaw_rate_target,double time);
            void UpdateBFFVelocity(VelDoubles velocity);
            void RequestNewVelocity(VelDoubles Velocity);
            void RequestAccelerations();
            void RequestAllStop();
        private:
            void PrintRuntime();
            void PrintAccelerations();
    };
}