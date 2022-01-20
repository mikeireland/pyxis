
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

DFRobot_INA219_IIC     ina2193(&Wire, INA219_I2C_ADDRESS3);
DFRobot_INA219_IIC     ina2194(&Wire, INA219_I2C_ADDRESS4);

float ina219Reading3_mA = 341;
float extMeterReading3_mA = 342;
float ina219Reading4_mA = 513;
float extMeterReading4_mA = 518;

/*
 * Serial monitor set at 115200 Baud
 * Currently uses carriage return (can edit for other newline characters)
 * Address 3 is motors etc. 
 * Address 4 is computer
 * 
 * BusVoltage = Voltage of power source in V
 * ShuntVoltage = Internal voltage of INA219 in V
 * Current = Current in mA
 * Power = Power in mW
 */

void setup(void)
{
    Serial.begin(115200);
    while(!Serial);

    Serial.println();
    while((ina2194.begin() != true)||(ina2193.begin() != true)) {
        Serial.println("INA2194 begin failed");
        delay(2000);
    }
    ina2193.linearCalibrate(ina219Reading3_mA, extMeterReading3_mA);
    ina2194.linearCalibrate(ina219Reading4_mA, extMeterReading4_mA);
    Serial.println();
}


void loop() {// Press enter in the serial monitor when running to receive the reading
  
  byte ch;

  while (Serial.available()) {

    ch = Serial.read();

    if (ch == '\r') { // Command recevied and ready. Change if not using carriage return.
      Serial.println("-----------------------");
      Serial.print("Address 3 (Motors) BusVoltage:   ");
      Serial.print(ina2193.getBusVoltage_V(), 2);
      Serial.println("V");
      Serial.print("Address 3 (Motors) ShuntVoltage: ");
      Serial.print(ina2193.getShuntVoltage_mV(), 3);
      Serial.println("mV");
      Serial.print("Address 3 (Motors) Current:      ");
      Serial.print(ina2193.getCurrent_mA(), 1);
      Serial.println("mA");
      Serial.print("Address 3 (Motors) Power:        ");
      Serial.print(ina2193.getPower_mW(), 1);
      Serial.println("mW");
      Serial.println("");
      Serial.print("Address 4 (Computer) BusVoltage:   ");
      Serial.print(ina2194.getBusVoltage_V(), 2);
      Serial.println("V");
      Serial.print("Address 4 (Computer) ShuntVoltage: ");
      Serial.print(ina2194.getShuntVoltage_mV(), 3);
      Serial.println("mV");
      Serial.print("Address 4 (Computer) Current:      ");
      Serial.print(ina2194.getCurrent_mA(), 1);
      Serial.println("mA");
      Serial.print("Address 4 (Computer) Power:        ");
      Serial.print(ina2194.getPower_mW(), 1);
      Serial.println("mW");
      Serial.println("-----------------------");
    }
  }
}
