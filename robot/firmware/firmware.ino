#include <SPI.h>

struct pitch_roll {
  double pitch, roll;
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

  const double k_lin_baseline = 500.0;
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
  MotorDriver(){
    for (int i = 0; i < 7; ++i) {
      pinMode(step_pins[i], OUTPUT);
      pinMode(dir_pins[i], OUTPUT);
    }
  }
  
  double motor_vels[7] = {0};

  // Sets the motor velocities based on the Body Fixed Frame velocity
  void set_velocities(BFF_velocities vels) {

    noInterrupts();
    
    motor_vels[0] = (                    - vels.y            ) * k_trans - vels.yaw * k_yaw;
    motor_vels[1] = ( vels.x * sin(PI/3) + vels.y * cos(PI/3)) * k_trans - vels.yaw * k_yaw;
    motor_vels[2] = (-vels.x * sin(PI/3) + vels.y * cos(PI/3)) * k_trans - vels.yaw * k_yaw;
    motor_vels[3] =   vels.roll  * k_lin_baseline - vels.pitch * (k_lin_baseline/2) + vels.z * k_z;
    motor_vels[4] =  -vels.roll  * k_lin_baseline - vels.pitch * (k_lin_baseline/2) + vels.z * k_z;
    motor_vels[5] =                               + vels.pitch * (k_lin_baseline  ) + vels.z * k_z;
    interrupts();
    
  }

  void set_raw_velocity(int motor_index, float v){
    motor_vels[motor_index] = v;
  }

  // This function checks if a step command should be sent to the motors, and updates them if so.
  // It should be called regularly to ensure smooth operation of the motors, however it will
  void update_positions() {
    for (int i(0); i < 7; ++i) {
      unsigned long time_now = micros();
      digitalWrite(dir_pins[i], motor_vels[i]/vel_scales[i] > 0);

      if (motor_vels[i] == 0) {
        last_step_micros[i] = time_now;
        continue;
      }

      long dt = abs(vel_scales[i] / motor_vels[i]);

      if (last_step_micros[i] + dt <= time_now) {
        digitalWrite(step_pins[i], !digitalRead(step_pins[i]));
        last_step_micros[i] += dt;
      }
    }
  }

};

class AccelerometerReader{
private:

  // Constants for comminucating with the accelerometer
  const int read_byte = 0x01;
  const int write_byte = 0x00;
  
  const int dev_id_ad_reg = 0x00;
  const int dev_id_mst_reg = 0x01;
  const int part_id_reg = 0x02;
  const int temp_2_reg = 0x06;
  
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

  const float temp_bias = 2078.25;
  const float temp_slope = -9.05;

  const double gravity_x = 0;
  const double gravity_y = 0; 
  const double gravity_z = 16110;

  const double gravity_conversion_factor = 9.81/sqrt(gravity_x*gravity_x + gravity_y*gravity_y + gravity_z*gravity_z);
    
  const int SPI_max_rate = 10000000;
  
  static const int num_accelerometers = 1;

  // Add entries here to add more accelerometers
  const int chip_select_pins[num_accelerometers] = {10};

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

  
  float get_temp(int index){
    float temp = (read_SPI_byte(index, temp_2_reg)*256);
    return (temp - temp_bias) / temp_slope;
  } 

  
public:
  AccelerometerReader(){

    SPI.beginTransaction(SPISettings(SPI_max_rate, MSBFIRST, SPI_MODE0));
    SPI.begin();
    for(int i = 0; i < num_accelerometers; i++){

      pinMode(chip_select_pins[i], OUTPUT);

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
    }
  }

    // Measure the orientation of the specified accelerometer
    pitch_roll getPitchRoll(int index){
      
      int16_t x = ((int16_t)read_SPI_byte(index, x_data_3_reg)) << 8;
      x += read_SPI_byte(index, x_data_2_reg);

      int16_t y = ((int16_t)read_SPI_byte(index, y_data_3_reg)) << 8;
      y += read_SPI_byte(index, y_data_2_reg);
      
      int16_t z = ((int16_t)read_SPI_byte(index, z_data_3_reg)) << 8;
      z += read_SPI_byte(index, z_data_2_reg);
      
      return (pitch_roll){
          .pitch = -atan2((double)x, z),
          .roll = -atan2((double)y, z)
      };
  }

      // Get the acceleration in m/s^2 with gravity removed from the reading
      triple getAllAxesAcceleration(int index){
      
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

class Controller{
  MotorDriver motor_driver;
  AccelerometerReader accelerometer_reader;
  BFF_velocities v;
  pitch_roll p_r;
  float input_velocity_constant = 10;
  bool stabilise = false;

  void stabilise_platform_velocities(){
    p_r = accelerometer_reader.getPitchRoll(0);
    v.pitch = p_r.pitch;
    v.roll = p_r.roll;
  }

  void sweep_test(int index){
    const double speed_mult = 50;
    long start_t, stop_t, now_t;
  
    const int array_length = 24000;
  
  
    for (double freq = 1 /*Hz*/; freq <= 20; freq += 0.5) {
      double test_time = 120/freq;
      int downsample = 500/freq;   
      
      if (Serial.available()){
        Serial.print("Testing frequency: ");
        Serial.println(freq);
      }
      start_t = micros();
      now_t = start_t;
      stop_t =  start_t + test_time * 1000000.0;
  
      float t[array_length] = {0};
      float x[array_length] = {0};
      float y[array_length] = {0};
      float z[array_length] = {0};
      int i = -1;
      int d = 0;
      
      while (now_t < stop_t){
        now_t = micros();
        double speed_ = speed_mult * sin(2 * PI * (now_t-start_t) * freq / 1000000);
        set_raw_velocity(index, speed_);
  
        triple acc = getAllAxesAcceleration();
  
        if (d % downsample == 0) {
          i += 1;
          if (i >= array_length) { break; }
        }
        // Record data
        t[i] += now_t / ((double) downsample);
        x[i] += acc.x / ((double) downsample);
        y[i] += acc.y / ((double) downsample);
        z[i] += acc.z / ((double) downsample);
        ++d;
        
      }
  
      set_raw_velocity(index, 0);
    
      Serial.println("SEND START;");
  
      Serial.print("f;");
      Serial.print(freq, 10);
      Serial.print(";");
      Serial.println();
      
      Serial.print("t");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(t[j]);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("x");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(x[j], 10);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("y");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(y[j], 10);
        Serial.print(";");
      }
      Serial.println();
  
      Serial.print("z");
      Serial.print(";");
      for (int j = 0; j < i; ++j) {
        Serial.print(z[j], 10);
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

  void set_velocities(BFF_velocities v){
    motor_driver.set_velocities(v);
  }
  
  void set_raw_velocity(int motor_index, float v){
    motor_driver.set_raw_velocity(motor_index, v);
  }

  void update_positions(){
    motor_driver.update_positions();
  }

  void wait_for_commands() {
    
    if(Serial.available()){
     
      switch (Serial.read()){
          case '1':
            if (stabilise == true){
              Serial.println("Stabilisation Disabled");
              stabilise = false;
            }
            else {
              Serial.println("Stabilisation Enabled");
              stabilise = true;
            }
            break;
          case '2':
            triple a;
            a = accelerometer_reader.getAllAxesAcceleration(0);
            Serial.println(a.x, 10);
            Serial.println(a.y, 10);
            Serial.println(a.x, 10);
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
            v.x = 0;
            v.y = 0;
            v.yaw = 0;
            v.pitch = 0;
            v.roll = 0;
            v.z = 0;
            sweep_test(int(Serial.read()-48));
            break;
          default:
            Serial.println("BAD CONTROLS");
            break;
      }
      
    }

    if (stabilise == true){
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

void setup() {
  // Calls controller.update_positions regularly
  // This ensures that the motors can move smoothly
  timer.begin(pos_update, 5);
}


void loop() {
   controller.wait_for_commands();  
}
