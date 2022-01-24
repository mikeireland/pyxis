#include <Wire.h>
#include "../power_module/DFRobot_INA219.h"

class PowerReader {
  private:
    DFRobot_INA219_IIC ina2193(&Wire, INA219_I2C_ADDRESS3);
    DFRobot_INA219_IIC ina2194(&Wire, INA219_I2C_ADDRESS4);

    float ina219Reading3_mA = 341;
    float extMeterReading3_mA = 342;
    float ina219Reading4_mA = 513;
    float extMeterReading4_mA = 518;

  public:
    PowerReader() {
        while((ina2194.begin() != true)||(ina2193.begin() != true)) {
            delay(2000);
        }
        ina2193.linearCalibrate(ina219Reading3_mA, extMeterReading3_mA);
        ina2194.linearCalibrate(ina219Reading4_mA, extMeterReading4_mA);
    }

    GetMotorVoltage(byte buff[2]) {
        uint16_t data = ina2193.getBusVoltageRaw();
        buff[0] = data & 0x0F;
        buff[1] = data & 0xF0;
    }

    GetComputerVoltage(byte buff[2]) {
        uint16_t data = ina2194.getBusVoltageRaw();
        buff[0] = data & 0x0F;
        buff[1] = data & 0xF0;
    }

    GetMotorCurrent(byte buff[2]) {
        uint16_t data = ina2193.getCurrentRaw();
        buff[0] = data & 0x0F;
        buff[1] = data & 0xF0;
    }

    GetComputerCurrent(byte buff[2]) {
        uint16_t data = ina2194.getCurrentRaw();
        buff[0] = data & 0x0F;
        buff[1] = data & 0xF0;
    }


