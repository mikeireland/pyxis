//Establish some macros relevant for communicating with this particular accelerometer
#define read_byte 0x01
#define write_byte 0x00

#define dev_id_ad_reg 0x00
#define dev_id_mst_reg 0x01
#define part_id_reg 0x02
#define temp_2_reg 0x06
#define temp_1_reg 0x07

#define x_data_3_reg 0x08
#define x_data_2_reg 0x09
#define y_data_3_reg 0x0B
#define y_data_2_reg 0x0C
#define z_data_3_reg 0x0E
#define z_data_2_reg 0x0F

#define range_reg 0x2C
#define range_2g 0x01
#define measure_mode 0x04
#define power_ctl_reg 0x2D

struct Triple {
  byte x[2], y[2], z[2];
};

class AccelerometerReader {
  private:

    //Constants for temperature reading
    const float temp_offset_ = 25.0;
    const float temp_bias_ = 1885; //was 2078.25
    const float temp_slope_ = -9.05;

    //Values needed for converting to physical units
    const double gravity_x_ = 0;
    const double gravity_y_ = 0;
    const double gravity_z_ = 16110;

    const double gravity_conversion_factor_ = 9.81 / sqrt(gravity_x_ * gravity_x_ + gravity_y_ * gravity_y_ + gravity_z_*gravity_z_);

    const int SPI_max_rate_ = 10000000; // was 10000000

    static const int num_accelerometers_ = 6;

    // Add entries here to add more accelerometers
    // Bottom 3 accelerometers: 8,9,10; Top 3 accelerometers: 5,6,7;
    const int chip_select_pins_[num_accelerometers_] = {8, 9, 10, 5, 6, 7};

    unsigned int ReadRegister(int index, byte this_register) {
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((this_register << 1) | read_byte);
      unsigned int result = SPI.transfer(0x00);
      digitalWrite(chip_select_pins_[index], HIGH);
      return result;
    }

    void WriteRegister(int index, byte this_register, byte this_value) {
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((this_register << 1) | write_byte);
      SPI.transfer(this_value);
      digitalWrite(chip_select_pins_[index], HIGH);
    }

    byte ReadSPIByte(int index, byte this_register) {
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((this_register << 1) | read_byte);
      byte data = SPI.transfer(0x00);
      digitalWrite(chip_select_pins_[index], HIGH);
      return data;
    }

    short int ReadSPI2Bytes(int index, byte this_register) {
      byte byte_temp = 0; 

      //Enable the correct device to read from
      digitalWrite(chip_select_pins_[index], LOW);

      //Send the device the register we are reading from
      //The shift is because the register must be in the first 7 bits and the LSB is the Read or Write command
      SPI.transfer((this_register << 1) | read_byte);

      //Read the value of the register, this is done by sendin a 0x00
      short int result =  SPI.transfer(0x00) << 8;
      //Read the next value in the register and combine the two values via a bitwise OR
      byte_temp = SPI.transfer(0x00);    
      result = result | byte_temp;

      //Disable the device
      digitalWrite(chip_select_pins_[index], HIGH);
      return result;
    }



    float GetTemp(int index) {
      float temp = (ReadSPIByte(index, temp_2_reg) * 256);
      temp += (float)ReadSPIByte(index, temp_1_reg);
      return temp_offset_ + (temp - temp_bias_) / temp_slope_;
    }


  public:
    AccelerometerReader() {
      SPI.beginTransaction(SPISettings(SPI_max_rate_, MSBFIRST, SPI_MODE0));
      SPI.begin();
      
        for (int i = 0; i < num_accelerometers_; i++) {

          pinMode(chip_select_pins_[i], OUTPUT);
          digitalWrite(chip_select_pins_[i], HIGH);

          WriteRegister(i, range_reg, range_2g);
          WriteRegister(i, power_ctl_reg, measure_mode);
        }
       
    }

    /*
    // Measure the orientation of the specified accelerometer
    PitchRoll GetPitchRoll(int index) {

      short int x = ((short int)ReadSPIByte(index, x_data_3_reg)) << 8;
      x += ReadSPIByte(index, x_data_2_reg);

      short int y = ((short int)ReadSPIByte(index, y_data_3_reg)) << 8;
      y += ReadSPIByte(index, y_data_2_reg);

      short int z = ((short int)ReadSPIByte(index, z_data_3_reg)) << 8;
      z += ReadSPIByte(index, z_data_2_reg);

      return (PitchRoll) {
        .x = x,
         .y = y,
          .z = z
      };
    }
    */

    // Get the 16 most significant bits from an acclerometer and store them in a buffer which I pass. (I'm moving the decode process to the NUC N.B)
    // For reference I'll keep the older firmware around (but in a separate file)
    void GetX(int index,byte buff[2]){
      //Do a 2 byte read from the x register of the accelerometer
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((x_data_3_reg << 1) | read_byte);
      buff[0] =  SPI.transfer(0x00);
      buff[1] = SPI.transfer(0x00);    
      digitalWrite(chip_select_pins_[index], HIGH);
    }

    void GetY(int index,byte buff[2]){
      //Do a 2 byte read from the x register of the accelerometer
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((y_data_3_reg << 1) | read_byte);
      buff[0] =  SPI.transfer(0x00);
      buff[1] = SPI.transfer(0x00);    
      digitalWrite(chip_select_pins_[index], HIGH);
    }

    void GetZ(int index,byte buff[2]){
      //Do a 2 byte read from the x register of the accelerometer
      digitalWrite(chip_select_pins_[index], LOW);
      SPI.transfer((z_data_3_reg << 1) | read_byte);
      buff[0] =  SPI.transfer(0x00);
      buff[1] = SPI.transfer(0x00);    
      digitalWrite(chip_select_pins_[index], HIGH);
    }
};
