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
            Servo::Navigator navigator;

            Servo::Doubles BFF_velocity_target_; //We take these to be x,y,z = vx,vy,yaw 
            Servo::Doubles motor_velocities_target_; //For these take x,y,z == 0,1,2
            Servo::Doubles actuator_velocity_target_; //For these take x,y,z == 0,1,2

            //Motion in the plane
            void LinearLateralRamp(Servo::Doubles velocity_target,double time);
            void LinearYawRamp(double yaw_rate_target,double time);
            void UpdateBFFVelocity(Servo::Doubles velocity);

            //Actuator control
            void UpdateZVelocity(double velocity);
            void UpdateActuatorVelocity(Servo::Doubles velocity);
            void LinearActuatorRamp(Servo::Doubles velocity_initial, Servo::Doubles velocity_target, double time);
            void LowerToReference();

            //Leveller Control
            void EngageLeveller();
            void LevellerLoop();
            void PassAccelBytesToLeveller();
            void PassActuatorStepsToLeveller();

            //Navigator Control
            void EngageNavigator();
            void NavigatorLoop();
            void SetNewTargetPosition(Servo::Doubles position_target);
            void RequestNewVelocity(Servo::Doubles velocity);
            void PassMotorStepsToNavigator();
            void NavigatorTest();

            //General control
            void RequestAccelerations();
            void RequestAllStop();
            void RequestStepCounts();
            void RequestResetStepCount();

            //Debugging and I/O
            void LinearSweepTest();
            void RaiseAndLowerTest();
            void WriteLevellerStateToFile();
            void MeasureOrientationMeasurementNoise();

            //Resonance Testing
            //NOTE: in these subroutines, all frequencies are measured in milliHertz
            void ApplySinusoidalVelocity(Servo::Doubles velocity_amplitude, unsigned int frequency, unsigned int time);
            void WriteSinusoidalAccelerationsToFile(unsigned int frequency);
            void SinusoidalSweep(unsigned int start_frequency, unsigned int end_frequency, unsigned int step_size);
            bool sinusoidal_file_open_flag_ = false;
            
        private:
            //debugging
            void PrintRuntime();
            void PrintAccelerations();
            void PrintStepCounts();

            //I/O and timing data members
            bool leveller_file_open_flag_ = false;
    };


}
