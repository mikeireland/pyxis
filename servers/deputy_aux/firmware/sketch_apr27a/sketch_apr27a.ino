
IntervalTimer timer;
IntervalTimer messageChecker;

volatile bool ready = false;
volatile int diff = 0;
volatile int prev = 0;

byte latest_message = 0x00;

#define START 0xFF
#define EMPTY 0x00 //The empty command
#define TID 0x04
#define VOLTAGE 0x21
#define COMPASS 0x22
#define LEDON 0x31
#define LEDOFF 0x32

#include <Wire.h>
#include "DFRobot_INA219.h"


unsigned short int DeviceID  = 128;
unsigned short int FirmwareV = 2;

DFRobot_INA219_IIC     ina2193(&Wire1, INA219_I2C_ADDRESS3);
DFRobot_INA219_IIC     ina2194(&Wire1, INA219_I2C_ADDRESS4);

float ina219Reading3_mA = 341;
float extMeterReading3_mA = 342;
float ina219Reading4_mA = 513;
float extMeterReading4_mA = 518;

#include "MMC5883.h"

MMC5883MA MMC5883(Wire);

int ledPin = 15;

byte read_buffer_[64] = {0x00};
byte write_buffer_[64] = {0x00};

void writeMessage() {
  Serial.write(write_buffer_, 64);
  Serial.send_now();
  for (int i = 0; i < 64; i++) {
    write_buffer_[i] = 0x00;
  }
}

void readMessage() {
  if (Serial.available()) {
    int i = 0;
    while (Serial.available()) {
      read_buffer_[i] = Serial.read();
      i += 1;
    }
    i = 0;
    while (i < 64) {
      switch(read_buffer_[i]) {
        case START:
          i += 1;
          break;
        case TID:
          digitalWrite(ledPin, HIGH);
          write_buffer_[0] = TID;
          write_buffer_[1] = DeviceID;
          write_buffer_[2] = FirmwareV;
          writeMessage();
          i += 3;
          break;
        case VOLTAGE:
        //todo
          i+= 3;
          break;
        case COMPASS:
        //todo
          i += 3;
          break;
        case LEDON:
          digitalWrite(ledPin, HIGH);
          write_buffer_[0] = LEDON;
          writeMessage();
          i += 3;
          break;
        case LEDOFF:
          digitalWrite(ledPin, LOW);
          write_buffer_[0] = LEDOFF;
          writeMessage();
          i += 3;
          break;
        case EMPTY:
          i = 64;
          break;
        default:
          i += 1;
          break;
      }
    }
  }
}


void sync() {
  ready = true;
}

void setup() {
  // all init funcs

  pinMode(ledPin, OUTPUT);
  
  Serial.begin(115200);
  while(!Serial);

  // setup wattmeters
  while((ina2194.begin() != true)||(ina2193.begin() != true)) {

      delay(2000);
  }
  ina2193.linearCalibrate(ina219Reading3_mA, extMeterReading3_mA);
  ina2194.linearCalibrate(ina219Reading4_mA, extMeterReading4_mA);

  // setup magnetometer
  Wire.setClock(1000000);
  Wire.begin();
  MMC5883.begin();
  MMC5883.calibrate();
  
  timer.begin(sync, 5000);
  messageChecker.begin(readMessage, 100);
}

void loop() {
  // put your main code here, to run repeatedly:
  noInterrupts();
  ready = false;
  interrupts();
  
  //int now = micros();
  //diff = now - prev;
  //prev = now;
  //Serial.println(diff);
  
  //Serial.println(ina2193.getBusVoltage_V(), 2);
  //Serial.println(ina2194.getBusVoltage_V(), 2);
  //add to packet

  //Serial.println(MMC5883.readData());
  //add to packet


  
  //writeMessage();
  
  while (!ready) {
    delayMicroseconds(10);
  }

}
