//Controller.h
#pragma once
#include "SerialPort.h"
#include "Servo.h"


namespace Control 
{
    class RobotDriver
    {
        public:
            Comms::SerialPort teensy_port;
            Servo::Leveller leveller;

            Servo::VelDoubles BFF_velocity_target_; //We take these to be x,y,z = vx,vy,yaw 
            Servo::VelDoubles motor_velocities_target_; //For these take x,y,z == 0,1,2
            Servo::VelDoubles actuator_velocity_target_; //For these take x,y,z == 0,1,2

            //Motion in the plane
            void LinearSweepTest();
            void LinearLateralRamp(Servo::VelDoubles velocity_target,double time);
            void LinearYawRamp(double yaw_rate_target,double time);
            void UpdateBFFVelocity(Servo::VelDoubles velocity);
            void RequestNewVelocity(Servo::VelDoubles velocity);

            //Actuator control
            void RaiseAndLowerTest();
            void EngageLeveller();
            void UpdateZVelocity(double velocity);
            void UpdateActuatorVelocity(Servo::VelDoubles velocity);
            void PassAccelBytesToLeveller();


            //Debugging and general control
            void RequestAccelerations();
            void RequestAllStop();

        private:
            //debugging
            void PrintRuntime();
            void PrintAccelerations();
    };
}