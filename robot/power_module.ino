
/*!

   https://wiki.dfrobot.com/Gravity%3A%20I2C%20Digital%20Wattmeter%20SKU%3A%20SEN0291
   file getVoltageCurrentPower.ino
   SEN0291 Gravity: I2C Digital Wattmeter
   The module is connected in series between the power supply and the load to read the voltage, current and power
   The module has four I2C, these addresses are:
   INA219_I2C_ADDRESS1  0x40   A0 = 0  A1 = 0
   INA219_I2C_ADDRESS2  0x41   A0 = 1  A1 = 0
   INA219_I2C_ADDRESS3  0x44   A0 = 0  A1 = 1
   INA219_I2C_ADDRESS4  0x45   A0 = 1  A1 = 1

   Copyright    [DFRobot](https://www.dfrobot.com), 2016
   Copyright    GNU Lesser General Public License
   version  V0.1
   date  2019-2-27
*/

#include <Wire.h>
#include "DFRobot_INA219.h"

DFRobot_INA219_IIC     ina2194(&Wire, INA219_I2C_ADDRESS4);
DFRobot_INA219_IIC     ina2193(&Wire, INA219_I2C_ADDRESS3);

float ina219Reading_mA = 1000;
float extMeterReading_mA = 1000;

void setup(void)
{
    Serial.begin(115200);
    while(!Serial);

    Serial.println();
    while((ina2194.begin() != true)||(ina2193.begin() != true)) {
        Serial.println("INA2194 begin failed");
        delay(2000);
    }
    ina2194.linearCalibrate(ina219Reading_mA, extMeterReading_mA);
    ina2193.linearCalibrate(ina219Reading_mA, extMeterReading_mA);
    Serial.println();
}

void loop(void)
{
    Serial.print("Address 3 BusVoltage:   ");
    Serial.print(ina2193.getBusVoltage_V(), 2);
    Serial.println("V");
    Serial.print("Address 3 ShuntVoltage: ");
    Serial.print(ina2193.getShuntVoltage_mV(), 3);
    Serial.println("mV");
    Serial.print("Address 3 Current:      ");
    Serial.print(ina2193.getCurrent_mA(), 1);
    Serial.println("mA");
    Serial.print("Address 3 Power:        ");
    Serial.print(ina2193.getPower_mW(), 1);
    Serial.println("mW");
    Serial.println("");
    Serial.print("Address 4 BusVoltage:   ");
    Serial.print(ina2194.getBusVoltage_V(), 2);
    Serial.println("V");
    Serial.print("Address 4 ShuntVoltage: ");
    Serial.print(ina2194.getShuntVoltage_mV(), 3);
    Serial.println("mV");
    Serial.print("Address 4 Current:      ");
    Serial.print(ina2194.getCurrent_mA(), 1);
    Serial.println("mA");
    Serial.print("Address 4 Power:        ");
    Serial.print(ina2194.getPower_mW(), 1);
    Serial.println("mW");
    Serial.println("");
    Serial.println("");
    delay(1000);
}
