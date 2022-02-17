//RobotDriver.h
#ifndef ROBOT_DRIVER_H_INCLUDE_GUARD
#define ROBOT_DRIVER_H_INCLUDE_GUARD

#include "SerialPort.h"
#include "Servo.h"
#include "Decode.h"

// C library headers
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <chrono>

// Linux headers
#include <unistd.h> // write(), read(), close()


namespace Control 
{
    class RobotDriver
    {
        public:
            Comms::SerialPort teensy_port;
            Servo::Leveller leveller;
            Servo::Navigator navigator;
            Servo::Stabiliser stabiliser;

            Servo::Doubles BFF_velocity_target_; //We take these to be x,y,z = vx,vy,yaw 
            Servo::Doubles motor_velocities_target_; //For these take x,y,z == 0,1,2
            Servo::Doubles actuator_velocity_target_; //For these take x,y,z == 0,1,2

            //Motion in the plane
            void LinearLateralRamp(Servo::Doubles velocity_target,double time);
            void LinearYawRamp(double yaw_rate_target,double time);
            void UpdateBFFVelocity(Servo::Doubles velocity);
            void RequestNewVelocity(Servo::Doubles velocity);

            //Actuator control
            void UpdateZVelocity(double velocity);
            void UpdateActuatorVelocity(Servo::Doubles velocity);
            void LinearActuatorRamp(Servo::Doubles velocity_initial, Servo::Doubles velocity_target, double time);
            void LowerToReference();

            //Leveller Control
            void EngageLeveller();
            void SetNewTargetAngle(double pitch,double roll);
            void LevellerLoop();
            void LevellerSubLoop();
            void PassAccelBytesToLeveller();
            void PassActuatorStepsToLeveller();

            //Navigator Control
            void EngageNavigator();
            void NavigatorLoop();
            void SetNewTargetPosition(Servo::Doubles position_target);
            void PassMotorStepsToNavigator();
            void NavigatorTest();

            //Stabiliser Control
            void StabiliserSetup();
            void StabiliserLoop();
            void EngageStabiliser();
            void SetNewStabiliserTarget(Servo::Doubles velocity_target, Servo::Doubles angle_target);
            void PassAccelBytesToStabiliser();
            void PassStepsToStabiliser();
            void StabiliserTest();
            //This flag indicates that after 20 seconds of running the leveller should be disabled and the stabiliser
            //should be engaged. It is for normalising the accelerometer measurements of the stabiliser.
            //The second flag indicates that the leveller should be started with a 0 pitch and 0 roll target before being 
            //set to target the true value with a reset step count
            bool short_level_flag_ = false;
            bool short_level_flag2_ = false;


            //General control
            void RequestAccelerations();
            void RequestAllStop();
            void RequestStepCounts();
            void RequestResetStepCount();

            //Debugging and I/O
            void LinearSweepTest();
            void RaiseAndLowerTest();
            void WriteLevellerStateToFile();
            void WriteStabiliserStateToFile();
            void MeasureOrientationMeasurementNoise();

            //Resonance Testing
            //NOTE: in these subroutines, all frequencies are measured in milliHertz
            void ApplySinusoidalVelocity(Servo::Doubles velocity_amplitude, 
                                         double amplitude_target, 
                                         unsigned int frequency, 
                                         unsigned int time,
                                         char sweep_class);
            void WriteSinusoidalAccelerationsToFile(unsigned int frequency,double amplitude,char sweep_class);
            void SinusoidalSweepLinear(unsigned int start_frequency, 
                                       unsigned int end_frequency, 
                                       unsigned int step_size,
                                       unsigned int power,
                                       unsigned int time,
                                       char direction,
                                       double max_amplitude);
            void SinusoidalSweepLog(unsigned int start_frequency, 
                                    unsigned int end_frequency, 
                                    double factor,
                                    unsigned int power,
                                    unsigned int time,
                                    char direction,
                                    double max_amplitude);
            void SinusoidalAmplitudeSweepLinear(unsigned int frequency,
                                                double amp_min,
                                                double amp_max,
                                                double sample_count,
                                                unsigned int time,
                                                char direction);
            bool sinusoidal_file_open_flag_ = false;

            void PrintRuntime();
            
        private:
            //debugging
            
            void PrintAccelerations();
            void PrintStepCounts();

            //I/O and timing data members
            bool leveller_file_open_flag_ = false;
            bool stabiliser_file_open_flag_ = false;
    };

    //Class to store properties of the real time acceleration tester
    class AccelerationTester
    {
        public:
          AccelerationTester();
          void UpdateVelocity();
          void WriteToFile();
          Servo::Doubles BFF_velocity_input_;
          double time_;

          Servo::Doubles acc0_latest_measurements_;
          Servo::Doubles acc1_latest_measurements_;
          Servo::Doubles acc2_latest_measurements_;
          Servo::Doubles acc3_latest_measurements_;
          Servo::Doubles acc4_latest_measurements_;
          Servo::Doubles acc5_latest_measurements_;

          Servo::Doubles acc0_ground_state_;
          Servo::Doubles acc1_ground_state_;
          Servo::Doubles acc2_ground_state_;
          Servo::Doubles acc3_ground_state_;
          Servo::Doubles acc4_ground_state_;
          Servo::Doubles acc5_ground_state_;

        private:
          int test_number_;
          static double dt_;

          double amplitude_ = 0;
          double frequency_ = 0;
          double slope_ = 0;
          

          double Sinusoid(double amplitude, double frequency, double time);
          double TriangleWave(double slope, double frequency, double time);
          double SquareWave(double amplitude, double frequency, double time);

          bool file_open_flag_ = false;
    };

}

#endif