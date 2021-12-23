//Controller.h
#pragma once
#include "SerialPort.h"
#include "Servo.h"

// C library headers
#include <stdio.h>
#include <string.h>
#include <math.h>

// Linux headers
#include <unistd.h> // write(), read(), close()

//Macro headers
#include "Commands.h"
#include "ErrorCodes.h"
#include "Decode.h"


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
            void LinearLateralRamp(Servo::VelDoubles velocity_target,double time);
            void LinearYawRamp(double yaw_rate_target,double time);
            void UpdateBFFVelocity(Servo::VelDoubles velocity);
            void RequestNewVelocity(Servo::VelDoubles velocity);

            //Actuator control
            void EngageLeveller();
            void UpdateZVelocity(double velocity);
            void UpdateActuatorVelocity(Servo::VelDoubles velocity);
            void LinearActuatorRamp(Servo::VelDoubles velocity_initial_, Servo::VelDoubles velocity_target, double time);
            void PassAccelBytesToLeveller();


            //General control
            void RequestAccelerations();
            void RequestAllStop();

            //Debugging and I/O
            void LinearSweepTest();
            void RaiseAndLowerTest();
            void WriteLevellerStateToFile();
            void MeasureOrientationMeasurementNoise();

        private:
            //debugging
            void PrintRuntime();
            void PrintAccelerations();

            //I/O and timing data members
            bool leveller_file_open_flag_ = false;
    };


}
