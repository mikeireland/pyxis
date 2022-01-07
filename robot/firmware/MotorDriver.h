class MotorDriver {
  private:
    // [(wheels), (linear actuators), goniometer]
    const int step_pins_[7] = {
      30, 32, 34,   // Wheels
      28, 26, 24,   // Linear actuators 0,1,2
      -1            // Goniometer
    };

    const int dir_pins_[7] = {
      31, 33, 35,   // Wheels
      29, 27, 25,   // Linear actuators 0,1,2
      -1            // Goniometer
    };

    const int lim_pins_[3] = {
      39,40,41 //Linear actuators 0,1,2
    };

    // Time (microseconds) of last update
    unsigned int last_step_micros_[7] = {0};
    //This array stores the physical distance which each motor moves per microstep
    //as an integer number of nanometres (the 1000s are placeholders for real values)
    //For the linear actuators the resolution is 0.01mm/step which (for 256 microsteps) is 39 nm/microstep
    unsigned int distance_per_microstep_[7] = {295,295,295,39,39,39,1000};

  public:
    MotorDriver() {
      for (int i = 0; i < 7; ++i) {
        pinMode(step_pins_[i], OUTPUT);
        pinMode(dir_pins_[i], OUTPUT);
      }
      for (int i = 0; i < 3; ++i) {
        pinMode(lim_pins_[i],INPUT_PULLUP);
      }
    }
    
    short int motor_vels_[7] = {0};
    bool pulse_needed_[7] = {false};
    int step_count_[7] = {0};

    void SetRawVelocity(int motor_index, short int v) {
      motor_vels_[motor_index] = v;
    }

    void ResetStepCount() {
      for(int i = 0; i < 7; i++) {
        step_count_[i] = 0;
      }
    }

    // This function checks if a step command should be sent to the motors, and updates them if so.
    // It should be called regularly to ensure smooth operation of the motors. Although the
    // motors only step on a low to high transition, we attempt to make a square wave here, i.e.
    // the step rate will be 1/dt, where dt is defined below.
    void UpdatePositions() {
      for (int i = 0; i < 7; ++i) {
        unsigned int time_now = micros();
        //Set the direction pins every time (even if velocity hasn't changed)
        //by the sign of the motor_vels_.
        
        digitalWrite(dir_pins_[i], motor_vels_[i] > 0);
        // If the velocity is zero, don't do anything!
        if (motor_vels_[i] == 0) 
        {
          last_step_micros_[i] = time_now;
          pulse_needed_[i] = false;
        }
        else
        {
          //Compute the step period
          int v = 1000*motor_vels_[i]; 
          unsigned int step_frequency = abs(15000*(v/32767))/distance_per_microstep_[i] ; //We compute the step frequency in Hz (to preserve some accuracy)
          
          if(step_frequency == 0) {
            last_step_micros_[i] = time_now;
            pulse_needed_[i] = false;
          } 
          
          else {
            unsigned int dt = 1000000/step_frequency; //Compute the step period in microseconds

            // We check if enough time has passed for a step to be needed and store this fact
            if (last_step_micros_[i] + dt <= time_now) {
              pulse_needed_[i] = true;
              last_step_micros_[i] += dt;
            }
            
            else {
            pulse_needed_[i] = false;
            }
          }
        }
      }
      //We pulse each motor with a 1microsec pulse if one is needed
      for (int i = 0; i < 7; ++i){
        if(pulse_needed_[i]){
          digitalWrite(step_pins_[i],HIGH);
          step_count_[i] = motor_vels_[i] > 0 ? step_count_[i] + 1 : step_count_[i] - 1; //update the step count
          pulse_needed_[i] = false;
        }
      }
      delayMicroseconds(1);
      for (int i = 0; i < 7; ++i){
        digitalWrite(step_pins_[i],LOW);
      }
    }

    //Checks if the limit switches on the actuators have been compressed and if so, sets the velocity of that linear actuator to 0
    void CheckLimitSwitches(){
      for(int i = 0; i < 3; ++i){
        if(digitalRead(lim_pins_[i]) && (motor_vels_[3 + i] > 0)){ //A positive actuator velocity is a lowering of the robot
          motor_vels_[3 + i] = 0;
        }
      }
    }
};
