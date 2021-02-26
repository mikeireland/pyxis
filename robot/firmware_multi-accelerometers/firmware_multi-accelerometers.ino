#include <SPI.h>

struct pitch_roll {
  double x, y, z;
};

struct triple {
  double x, y, z;
};

struct BFF_velocities {

  // metres / second
  double x = 0; // forwards W.R.T wheel 0
  double y = 0; // left W.R.T wheel 0
  double z = 0; // up

  // radians / second
  double pitch = 0;
  double yaw = 0;
  double roll = 0;

};

class MotorDriver {
  private:
    //k_lin_baseline somehow converts from angular to linear units. See Mike's PDF for correct units! Was 500.0. Why?
    const double k_lin_baseline = 1.0;
    const double k_z = 1.0;
    const double k_trans = 1.0;
    const double k_yaw = 1.0;

    // [(wheels), (linear actuators), goniometer]
    const int step_pins[7] = {
      30, 32, 34,   // Wheels
      24, 26, 28,   // Linear actuators
      -1            // Goniometer
    };

    const int dir_pins[7] = {
      31, 33, 35,   // Wheels
      25, 27, 29,   // Linear actuators
      -1            // Goniometer
    };

    // microseconds per step at v=1; negative sign reverses direction
    // German motor gearbox ratio = 25:1. Chinese motor gearbox ratio is 20:1.
    const double vel_scales[7] = {1000, 1000, 800, -1000, -1000, -1000, 1000};

    // Time (microseconds) of last update
    unsigned long last_step_micros[7] = {0};

  public:
    MotorDriver() {
      for (int i = 0; i < 7; ++i) {
        pinMode(step_pins[i], OUTPUT);
        pinMode(dir_pins[i], OUTPUT);
      }
    }

    double motor_vels[7] = {0};

    // Sets the motor velocities based on the Body Fixed Frame velocity
    void set_velocities(BFF_velocities vels) {

      noInterrupts();
      // PI/3 below is the angle of the linear actuator with respect to the X axis of the robot.
      motor_vels[0] = (                    - vels.y            ) * k_trans - vels.yaw * k_yaw;
      motor_vels[1] = ( vels.x * sin(PI / 3) + vels.y * cos(PI / 3)) * k_trans - vels.yaw * k_yaw;
      motor_vels[2] = (-vels.x * sin(PI / 3) + vels.y * cos(PI / 3)) * k_trans - vels.yaw * k_yaw;
      motor_vels[3] =   sin(PI / 3) * vels.roll  * k_lin_baseline - vels.pitch * k_lin_baseline * cos(PI / 3) + vels.z * k_z; // was not divided by 5 (?)
      motor_vels[4] =  -sin(PI / 3) * vels.roll  * k_lin_baseline - vels.pitch * k_lin_baseline * cos(PI / 3) + vels.z * k_z; // was not divided by 5 (?)
      motor_vels[5] =                               + vels.pitch * (k_lin_baseline  ) + vels.z * k_z;  // was not divided by 5
      interrupts();

    }

    void set_raw_velocity(int motor_index, float v) {
      motor_vels[motor_index] = v;
    }

    // This function checks if a step command should be sent to the motors, and updates them if so.
    // It should be called regularly to ensure smooth operation of the motors. Although the
    // motors only step on a low to high transition, we attempt to make a square wave here, i.e.
    // the step rate will be 0.5/dt, where dt is defined below.
    void update_positions() {
      for (int i(0); i < 7; ++i) {
        unsigned long time_now = micros();
        //Set the direction pins every time (even if velocity hasn't changed)
        //by the sign of the motor_vels relaive to the signe of vel_scales.
        digitalWrite(dir_pins[i], motor_vels[i] / vel_scales[i] > 0);

        // If the velocity is zero, don't do anything!
        if (motor_vels[i] == 0) {
          last_step_micros[i] = time_now;
          continue;
        }

        // Set our stepping rate, which is equal to 0.5/vel_scales * motor_vels
        long dt = abs(vel_scales[i] / motor_vels[i]);

        // If enough time has passed, step this motor!
        if (last_step_micros[i] + dt <= time_now) {
          digitalWrite(step_pins[i], !digitalRead(step_pins[i]));
          last_step_micros[i] += dt;
        }
      }
    }

};

class AccelerometerReader {
  private:

    // Constants for comminucating with the accelerometer
    const int read_byte = 0x01;
    const int write_byte = 0x00;

    const int dev_id_ad_reg = 0x00;
    const int dev_id_mst_reg = 0x01;
    const int part_id_reg = 0x02;
    const int temp_2_reg = 0x06;
    const int temp_1_reg = 0x07;

    const int x_data_3_reg = 0x08;
    const int x_data_2_reg = 0x09;
    const int y_data_3_reg = 0x0B;
    const int y_data_2_reg = 0x0C;
    const int z_data_3_reg = 0x0E;
    const int z_data_2_reg = 0x0F;

    const int range_reg = 0x2C;
    const int range_2g = 0x01;
    const int measure_mode = 0x04;
    const int power_ctl_reg = 0x2D;

    const float temp_offset = 25.0;
    const float temp_bias = 1885; //was 2078.25
    const float temp_slope = -9.05;

    const double gravity_x = 0;
    const double gravity_y = 0;
    const double gravity_z = 16110;

    const double gravity_conversion_factor = 9.81 / sqrt(gravity_x * gravity_x + gravity_y * gravity_y + gravity_z*gravity_z);

    const int SPI_max_rate = 10000000; // was 10000000

    static const int num_accelerometers = 6;

    // Add entries here to add more accelerometers
    // Bottom 3 accelerometers: 8,9,10; Top 3 accelerometers: 5,6,7;
    const int chip_select_pins[num_accelerometers] = {8, 9, 10, 5, 6, 7};

    unsigned int read_register(int index, byte this_register) {
      digitalWrite(chip_select_pins[index], LOW);
      SPI.transfer((this_register << 1) | read_byte);
      unsigned int result = SPI.transfer(0x00);
      digitalWrite(chip_select_pins[index], HIGH);
      return result;
    }

    void write_register(int index, byte this_register, byte this_value) {
      digitalWrite(chip_select_pins[index], LOW);
      SPI.transfer((this_register << 1) | write_byte);
      SPI.transfer(this_value);
      digitalWrite(chip_select_pins[index], HIGH);
    }

    byte read_SPI_byte(int index, byte this_register) {
      digitalWrite(chip_select_pins[index], LOW);
      SPI.transfer((this_register << 1) | read_byte);
      byte data = SPI.transfer(0x00);
      digitalWrite(chip_select_pins[index], HIGH);
      return data;
    }


    float get_temp(int index) {
      float temp = (read_SPI_byte(index, temp_2_reg) * 256);
      temp += (float)read_SPI_byte(index, temp_1_reg);
      return temp_offset + (temp - temp_bias) / temp_slope;
    }


  public:
    AccelerometerReader() {
      SPI.beginTransaction(SPISettings(SPI_max_rate, MSBFIRST, SPI_MODE0));
      SPI.begin();
      
        for (int i = 0; i < num_accelerometers; i++) {

          pinMode(chip_select_pins[i], OUTPUT);
          digitalWrite(chip_select_pins[i], HIGH);

          write_register(i, range_reg, range_2g);
          write_register(i, power_ctl_reg, measure_mode);

          while (!Serial) {}
          Serial.print("ACCELEROMETER INDEX: ");
          Serial.println(i);

          Serial.print("DEVID_AD: ");
          Serial.println(read_SPI_byte(i, dev_id_ad_reg), HEX);

          Serial.print("DEVID_MST: ");
          Serial.println(read_SPI_byte(i, dev_id_mst_reg), HEX);

          Serial.print("PART_ID: ");
          Serial.println(read_SPI_byte(i, part_id_reg), HEX);

          Serial.print("TEMP (C): ");
          Serial.println(get_temp(i));

          Serial.print("X3: ");
          Serial.println(read_SPI_byte(i, x_data_3_reg), HEX);
          Serial.print("X2: ");
          Serial.println(read_SPI_byte(i, x_data_2_reg), HEX);
          Serial.print("Y3: ");
          Serial.println(read_SPI_byte(i, y_data_3_reg), HEX);
          Serial.print("Y2: ");
          Serial.println(read_SPI_byte(i, y_data_2_reg), HEX);
          Serial.print("Z3: ");
          Serial.println(read_SPI_byte(i, z_data_3_reg), HEX);
          Serial.print("Z2: ");
          Serial.println(read_SPI_byte(i, z_data_2_reg), HEX);
        }
       
    }

    // Measure the orientation of the specified accelerometer
    pitch_roll getPitchRoll(int index) {

      int16_t x = ((int16_t)read_SPI_byte(index, x_data_3_reg)) << 8;
      x += read_SPI_byte(index, x_data_2_reg);

      int16_t y = ((int16_t)read_SPI_byte(index, y_data_3_reg)) << 8;
      y += read_SPI_byte(index, y_data_2_reg);

      int16_t z = ((int16_t)read_SPI_byte(index, z_data_3_reg)) << 8;
      z += read_SPI_byte(index, z_data_2_reg);

      return (pitch_roll) {
        .x = x,
         .y = y,
          .z = z
      };
    }

    // Get the acceleration in m/s^2 with gravity removed from the reading
    triple getAllAxesAcceleration(int index) {

      int16_t x = ((int16_t)read_SPI_byte(index, x_data_3_reg)) << 8;
      x += read_SPI_byte(index, x_data_2_reg);

      int16_t y = ((int16_t)read_SPI_byte(index, y_data_3_reg)) << 8;
      y += read_SPI_byte(index, y_data_2_reg);

      int16_t z = ((int16_t)read_SPI_byte(index, z_data_3_reg)) << 8;
      z += read_SPI_byte(index, z_data_2_reg);

      triple a;   // 1g = 16120
      a.x = (x - gravity_x) * gravity_conversion_factor;
      a.y = (y - gravity_y) * gravity_conversion_factor;
      a.z = (z - gravity_z) * gravity_conversion_factor;
      return a;
    }

};

class Controller {
    MotorDriver motor_driver;
    AccelerometerReader accelerometer_reader;
    BFF_velocities v;
    pitch_roll p_r;
    float input_velocity_constant = 10;
    bool stabilise = false;

    void stabilise_platform_velocities() {
      triple read_v;
      p_r = accelerometer_reader.getPitchRoll(0);
      read_v.x = -0.866 * p_r.x + 0.866 * p_r.y;
      read_v.y = -0.5 * p_r.x - 0.5 * p_r.y;
      read_v.z = p_r.z;


      p_r = accelerometer_reader.getPitchRoll(1);
      read_v.x = read_v.x + p_r.x;
      read_v.y = read_v.y - p_r.y;
      read_v.z = read_v.z + p_r.z;

      p_r = accelerometer_reader.getPitchRoll(2);
      read_v.x = read_v.x - 0.5 * p_r.x - 0.866 * p_r.y;
      read_v.y = read_v.y + 0.866 * p_r.x - 0.5 * p_r.y;
      read_v.z = read_v.z + p_r.z;

      read_v.x = read_v.x / 3;
      read_v.y = read_v.y / 3;
      read_v.z = read_v.z / 3;

      v.pitch = -atan2((double)read_v.x, read_v.z),
        v.roll = -atan2((double)read_v.y, read_v.z),

          Serial.print("read.x: ");
      Serial.print(read_v.x, 10);
      Serial.print(" | read.y: ");
      Serial.print(read_v.y, 10);
      Serial.print(" | read.z: ");
      Serial.println(read_v.z, 10);
      Serial.print("v.pitch: ");
      Serial.print(v.pitch, 10);
      Serial.print(" | v.roll: ");
      Serial.println(v.roll, 10);
      Serial.println(' ');
      delay(1000);
    }

    void sweep_test(char index) {
      const double speed_mult = 50;
      long start_t, stop_t, now_t;
      BFF_velocities BFF_sweep, BFF_zero; //The struct definition initializes all axes to zero.
      const int array_length = 24000; 
      // serial monitor will crush and restart if
      // the array length is too large

      for (double freq = 2 /*Hz*/; freq <= 30; freq += 2) {
        double test_time = 120 / freq; //!!! Was 120. Number of samples divided by frequency.
        int downsample = 500 / freq; // Number of samples toaverage divided by frequency.

        if (Serial.available()) {
          Serial.print("Testing frequency: ");
          Serial.println(freq);
        }
        start_t = micros();
        now_t = start_t;
        stop_t =  start_t + (long)(test_time * 1000000.0);


        float t[array_length] = {0};
        float x[array_length] = {0};
        float y[array_length] = {0};
        float z[array_length] = {0};
        int i = -1;
        int d = 0;

        // Initialize velocities to zero.
        set_velocities(BFF_zero);

        while (now_t < stop_t) {
          now_t = micros();
          double speed_ = speed_mult * sin(2 * PI * (now_t - start_t) * freq / 1000000);
          // Set the raw velocity if the motor index is '0' through '5'
          if ((index >= 48) && (index < 54))
            set_raw_velocity(index - 48, speed_);
          else if (index == 'x')
          {
            BFF_sweep.x = speed_;
            set_velocities(BFF_sweep);
          }
          else if (index == 'y')
          {
            BFF_sweep.y = speed_;
            set_velocities(BFF_sweep);
          }
          else if (index == 'z')
          {
            BFF_sweep.z = speed_;
            set_velocities(BFF_sweep);
          }
          else if (index == 'p')
          {
            BFF_sweep.pitch = speed_;
            set_velocities(BFF_sweep);
          }
          else if (index == 'r')
          {
            BFF_sweep.roll = speed_;
            set_velocities(BFF_sweep);
          }
          else if (index == 's') //"spin" for yaw
          {
            BFF_sweep.yaw = speed_;
            set_velocities(BFF_sweep);
          }
          
        triple a_bottom; triple a_top;
        triple a_axes;
        triple acc;
        //Explicit matrix multiplication. See PDF from Mike.
        
        //Measure bottom overall acceleration
        a_axes = accelerometer_reader.getAllAxesAcceleration(0);
        a_bottom.x = -0.866 * a_axes.x + 0.866 * a_axes.y;
        a_bottom.y = -0.5 * a_axes.x - 0.5 * a_axes.y;
        a_bottom.z = a_axes.z;

        a_axes = accelerometer_reader.getAllAxesAcceleration(1);
        a_bottom.x = a_bottom.x + a_axes.x;
        a_bottom.y = a_bottom.y - a_axes.y;
        a_bottom.z = a_bottom.z + a_axes.z;

        a_axes = accelerometer_reader.getAllAxesAcceleration(2);
        a_bottom.x = a_bottom.x - 0.5 * a_axes.x - 0.866 * a_axes.y;
        a_bottom.y = a_bottom.y + 0.866 * a_axes.x - 0.5 * a_axes.y;
        a_bottom.z = a_bottom.z + a_axes.z;

        a_bottom.x = a_bottom.x / 3;
        a_bottom.y = a_bottom.y / 3;
        a_bottom.z = a_bottom.z / 3;

        //Measure top overall acceleration
        a_axes = accelerometer_reader.getAllAxesAcceleration(3);
        a_top.x = a_axes.x;
        a_top.y = a_axes.y;
        a_top.z = a_axes.z;

        a_axes = accelerometer_reader.getAllAxesAcceleration(4);
        a_top.x = a_top.x - a_axes.y;
        a_top.y = a_top.y + a_axes.x;
        a_top.z = a_top.z + a_axes.z;

        a_axes = accelerometer_reader.getAllAxesAcceleration(5);
        a_top.x = a_top.x - a_axes.x;
        a_top.y = a_top.y - a_axes.y;
        a_top.z = a_top.z + a_axes.z;

        a_top.x = a_top.x / 3;
        a_top.y = a_top.y / 3;
        a_top.z = a_top.z / 3;

        //Average top and bottom acceleration
        acc.x = (a_bottom.x + a_top.x) / 2;
        acc.y = (a_bottom.x + a_top.y) / 2;
        acc.z = (a_bottom.x + a_top.z) / 2;

          if (d % downsample == 0) {
            i += 1;
            if (i >= array_length) { break;}
          }
          // Record data
        t[i] += now_t / ((double) downsample);
        x[i] += acc.x / ((double) downsample);
        y[i] += acc.y / ((double) downsample);
        z[i] += acc.z / ((double) downsample);
          ++d;
        }

        // Set all velocities to zero.
      set_velocities(BFF_zero);

      Serial.println("SEND START;");

      Serial.print("f;");
      Serial.print(freq, 10);
      Serial.print(";");
      Serial.println();
      
      Serial.print("t");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(t[j],0);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("x");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(x[j], 6);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("y");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(y[j], 6);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("z");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(z[j], 6);
        Serial.print(";");
      }
      Serial.println();
      
  
      Serial.println("SEND END;");
  
      delay(1000);

      }
      Serial.println("&");
    }

  public:

    triple getAllAxesAcceleration(int i = 0) {
      return accelerometer_reader.getAllAxesAcceleration(i);
    }

    void set_velocities(BFF_velocities v) {
      motor_driver.set_velocities(v);
    }

    void set_raw_velocity(int motor_index, float v) {
      motor_driver.set_raw_velocity(motor_index, v);
    }

    void update_positions() {
      motor_driver.update_positions();
    }

    void wait_for_commands() {
      if (Serial.available()) {
        switch (Serial.read()) {
          case '1':
            if (stabilise == true) {
              Serial.println("Stabilisation Disabled");
              stabilise = false;
            }
            else {
              Serial.println("Stabilisation Enabled");
              stabilise = true;
            }
            break;
          case '2':
            triple a_0; triple a_1; triple a_2; 
            triple a_3; triple a_4; triple a_5;
            triple a_bottom; triple a_top;
            
            a_0 = accelerometer_reader.getAllAxesAcceleration(0);
            a_bottom.x = -0.866 * a_0.x + 0.866 * a_0.y;
            a_bottom.y = -0.5 * a_0.x - 0.5 * a_0.y;
            a_bottom.z = a_0.z;

            a_1 = accelerometer_reader.getAllAxesAcceleration(1);
            a_bottom.x = a_bottom.x + a_1.x;
            a_bottom.y = a_bottom.y - a_1.y;
            a_bottom.z = a_bottom.z + a_1.z;

            a_2 = accelerometer_reader.getAllAxesAcceleration(2);
            a_bottom.x = a_bottom.x - 0.5 * a_2.x - 0.866 * a_2.y;
            a_bottom.y = a_bottom.y + 0.866 * a_2.x - 0.5 * a_2.y;
            a_bottom.z = a_bottom.z + a_2.z;

            a_bottom.x = a_bottom.x / 3;
            a_bottom.y = a_bottom.y / 3;
            a_bottom.z = a_bottom.z / 3;

            a_3 = accelerometer_reader.getAllAxesAcceleration(3);
            a_top.x = a_3.x;
            a_top.y = a_3.y;
            a_top.z = a_3.z;

            a_4 = accelerometer_reader.getAllAxesAcceleration(4);
            a_top.x = a_top.x - a_4.y;
            a_top.y = a_top.y + a_4.x;
            a_top.z = a_top.z + a_4.z;

            a_5 = accelerometer_reader.getAllAxesAcceleration(5);
            a_top.x = a_top.x - a_5.x;
            a_top.y = a_top.y - a_5.y;
            a_top.z = a_top.z + a_5.z;

            a_top.x = a_top.x / 3;
            a_top.y = a_top.y / 3;
            a_top.z = a_top.z / 3;
            
//            Serial.print("a_bottom.x = ");
//            Serial.print(a_bottom.x, 5);
//            Serial.print(" | a_bottom.y = ");
//            Serial.print(a_bottom.y, 5);
//            Serial.print(" | a_bottom.z = ");
//            Serial.println(a_bottom.z, 5);
//
//            Serial.print("a_top.x = ");
//            Serial.print(a_top.x, 5);
//            Serial.print(" | a_top.y = ");
//            Serial.print(a_top.y, 5);
//            Serial.print(" | a_top.z = ");
//            Serial.println(a_top.z, 5);
//            Serial.println(" ");

            Serial.print("a_0.x = ");
            Serial.print(a_0.x, 5);
            Serial.print(" | a_0.y = ");
            Serial.print(a_0.y, 5);
            Serial.print(" | a_0.z = ");
            Serial.println(a_0.z, 5);

            Serial.print("a_1.x = ");
            Serial.print(a_1.x, 5);
            Serial.print(" | a_1.y = ");
            Serial.print(a_1.y, 5);
            Serial.print(" | a_1.z = ");
            Serial.println(a_1.z, 5);

            Serial.print("a_2.x = ");
            Serial.print(a_2.x, 5);
            Serial.print(" | a_2.y = ");
            Serial.print(a_2.y, 5);
            Serial.print(" | a_2.z = ");
            Serial.println(a_2.z, 5);

            Serial.print("a_3.x = ");
            Serial.print(a_3.x, 5);
            Serial.print(" | a_3.y = ");
            Serial.print(a_3.y, 5);
            Serial.print(" | a_3.z = ");
            Serial.println(a_3.z, 5);

            Serial.print("a_4.x = ");
            Serial.print(a_4.x, 5);
            Serial.print(" | a_4.y = ");
            Serial.print(a_4.y, 5);
            Serial.print(" | a_4.z = ");
            Serial.println(a_4.z, 5);

            Serial.print("a_5.x = ");
            Serial.print(a_5.x, 5);
            Serial.print(" | a_5.y = ");
            Serial.print(a_5.y, 5);
            Serial.print(" | a_5.z = ");
            Serial.println(a_5.z, 5);
            
            Serial.println(" ");

            break;
          case 'z':
            Serial.println("Z up");
            v.z += input_velocity_constant;
            break;
          case 'x':
            Serial.println("Z down");
            v.z -= input_velocity_constant;
            break;
          case 'w':
            Serial.println("X Forward");
            v.x += input_velocity_constant;
            break;
          case 's':
            Serial.println("X Backward");
            v.x -= input_velocity_constant;
            break;
          case 'a':
            Serial.println("Y Forward");
            v.y += input_velocity_constant;
            break;
          case 'd':
            Serial.println("Y Backward");
            v.y -= input_velocity_constant;
            break;
          case 'q':
            Serial.println("Yaw Anticlockwise");
            v.yaw += input_velocity_constant;
            break;
          case 'e':
            Serial.println("Yaw Clockwise");
            v.yaw -= input_velocity_constant;
            break;
          case 't':
            Serial.println("Pitch Anticlockwise");
            v.pitch += input_velocity_constant;
            break;
          case 'y':
            Serial.println("Pitch Clockwise");
            v.pitch -= input_velocity_constant;
            break;
          case 'g':
            Serial.println("Roll Anticlockwise");
            v.roll += input_velocity_constant;
            break;
          case 'h':
            Serial.println("Roll Clockwise");
            v.roll -= input_velocity_constant;
            break;  
          case 'p':
            Serial.println("STOPPED");
            v.x = 0;
            v.y = 0;
            v.yaw = 0;
            v.pitch = 0;
            v.roll = 0;
            v.z = 0;
            break;
          case '\n':
            Serial.print("x:");
            Serial.print(v.x);
            Serial.print(" y:");
            Serial.print(v.y);
            Serial.print(" yaw:");
            Serial.print(v.yaw);
            Serial.print(" z:");
            Serial.println(v.z);
            break;
          case '+':
            sweep_test(Serial.read());
            break;
          default:
            Serial.println("BAD CONTROLS");
            break;
        }

      }

      if (stabilise == true) {
        stabilise_platform_velocities();
      }
      else {
        v.pitch = 0;
        v.roll = 0;
      }
      motor_driver.set_velocities(v);
    }
};

// Global instances
IntervalTimer timer;
Controller controller;

void pos_update() {
  controller.update_positions();
}

//int val = 0;
//int ledPin = 5;
//int inPin = 6;

void setup() {
  // Calls controller.update_positions regularly
  // This ensures that the motors can move smoothly
  timer.begin(pos_update, 5);
  //  pinMode(ledPin, OUTPUT);
  //  pinMode(inPin, INPUT);
}


void loop() {
  controller.wait_for_commands();
  //   val = digitalRead(inPin);   // read the input pin
  //   digitalWrite(ledPin, val);

}
