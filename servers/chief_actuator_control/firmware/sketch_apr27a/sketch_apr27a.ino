
#define START 0xFF
#define EMPTY 0x00 //The empty command
#define TID 0x04
#define WATTMETER 0x21
#define COMPASS 0x22
#define SETPWM 0x31 // requires 5x uint8_t args for duty cycles
#define GETPWM 0x32 
#define SETSDC 0x33 // int32_t steps, uint16_t period
#define GETSDC 0x34

#include <Wire.h>
#include "DFRobot_INA219.h"
#include "MMC5883.h"

// interval timers
IntervalTimer SDCStepper;
IntervalTimer messageChecker;
IntervalTimer piezoPWM;
IntervalTimer voltageReader;

// for timing test
volatile bool ready = false;
volatile int diff = 0;
volatile int prev = 0;


byte latest_message = 0x00;
byte read_buffer_[128] = {0x00};
byte write_buffer_[128] = {0x00};
volatile int write_index = 0;

unsigned short int DeviceID  = 128;
unsigned short int FirmwareV = 2;

DFRobot_INA219_IIC     ina2193(&Wire1, INA219_I2C_ADDRESS3);
DFRobot_INA219_IIC     ina2194(&Wire1, INA219_I2C_ADDRESS4);

float ina219Reading3_mA = 341;
float extMeterReading3_mA = 342;
float ina219Reading4_mA = 513;
float extMeterReading4_mA = 518;


MMC5883MA MMC5883(Wire);

int wattmeter_SDA = 17;
int wattmeter_SCL = 16;

int piezos[5] = {8,9,10,11,12};

int SDC_CLK = 41;
int SDC_DIR = 40;
int SDC_EN = 39;
int SDC_ONOFF = 38;
int SDC_LIMP = 37;
int SDC_LIMN = 36;

volatile uint8_t duty_cycles[5] = {0};

volatile int32_t steps_to_go = 400000;
volatile int32_t current_step = 0; 
volatile uint16_t period = 100;
int32_t limit = -400000;

int16_t PC_Voltage = 0;
int16_t PC_Current = 0;
int16_t Motor_Voltage = 0;
int16_t Motor_Current = 0;

void writeMessage() {
  Serial.write(write_buffer_, 64);
  Serial.send_now();
  for (int i = 0; i < 128; i++) {
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
    while (i < 128) {
      switch(read_buffer_[i]) {
        case START:
          i += 1;
          break;
        case TID:
          //digitalWrite(ledPin, HIGH);
          write_buffer_[write_index] = TID;
          write_buffer_[write_index + 1] = DeviceID;
          write_buffer_[write_index + 2] = FirmwareV;
          write_index += 3;
          //writeMessage();
          i += 3;
          break;
        case WATTMETER:
          write_buffer_[write_index] = WATTMETER;
          write_buffer_[write_index + 1] = PC_Voltage & 0xFF;
          write_buffer_[write_index + 2] = (PC_Voltage >> 8) & 0xFF;
          write_buffer_[write_index + 3] = PC_Current & 0xFF;
          write_buffer_[write_index + 4] = (PC_Current >> 8) & 0xFF;
          write_buffer_[write_index + 5] = Motor_Voltage & 0xFF;
          write_buffer_[write_index + 6] = (Motor_Voltage >> 8) & 0xFF;
          write_buffer_[write_index + 7] = Motor_Current & 0xFF;
          write_buffer_[write_index + 8] = (Motor_Current >> 8) & 0xFF;
          write_index += 9;
          //writeMessage();
          i+= 1;
          break;
        case COMPASS:
        //todo
          i += 3;
          break;
        case SETPWM:
          duty_cycles[0] = read_buffer_[i+1];
          duty_cycles[1] = read_buffer_[i+2];
          duty_cycles[2] = read_buffer_[i+3];
          duty_cycles[3] = read_buffer_[i+4];
          duty_cycles[4] = read_buffer_[i+5];
          i += 6;
          break;
        case GETPWM:
          write_buffer_[write_index] = GETPWM;
          write_buffer_[write_index + 1] = duty_cycles[0];
          write_buffer_[write_index + 2] = duty_cycles[1];
          write_buffer_[write_index + 3] = duty_cycles[2];
          write_buffer_[write_index + 4] = duty_cycles[3];
          write_buffer_[write_index + 5] = duty_cycles[4];
          write_index += 6;
          //writeMessage();
          i += 1;
          break;
        case SETSDC:
          period = ((uint16_t)read_buffer_[i+6] << 8) | (uint32_t)read_buffer_[i+5];
          SDCStepper.update(period);
          steps_to_go = ((uint32_t)read_buffer_[i+4] << 24) | ((uint32_t)read_buffer_[i+3] << 16) | ((uint32_t)read_buffer_[i+2] << 8) | (uint32_t)read_buffer_[i+1];
          i += 7;
          break;
        case GETSDC:
          write_buffer_[write_index] = GETSDC;
          write_buffer_[write_index + 1] = steps_to_go & 0xFF;
          write_buffer_[write_index + 2] = (steps_to_go >> 8) & 0xFF;
          write_buffer_[write_index + 3] = (steps_to_go >> 16) & 0xFF;
          write_buffer_[write_index + 4] = (steps_to_go >> 24) & 0xFF;
          write_buffer_[write_index + 5] = period & 0xFF;
          write_buffer_[write_index + 6] = (period >> 8) & 0xFF;
          write_index += 7;
          //writeMessage();
          i += 1;
          break;
        case EMPTY:
          i = 128;
          break;
        default:
          i += 1;
          break;
      }
    }
    if (write_index != 0) {
      writeMessage();
      write_index = 0;
    }
    for (int i = 0; i < 128; i++) {
      read_buffer_[i] = 0x00;
    }
  }
}

void update_piezos() {
  analogWrite(piezos[0], duty_cycles[0]);
  analogWrite(piezos[1], duty_cycles[1]);
  analogWrite(piezos[2], duty_cycles[2]);
  analogWrite(piezos[3], duty_cycles[3]);
  analogWrite(piezos[4], duty_cycles[4]);
}

void step_SDC() {
  // going negative
  if (steps_to_go < 0) {
    // not at negative limit
    if (current_step > limit) {
      digitalWrite(SDC_DIR, LOW);
      delayMicroseconds(10);
      digitalWrite(SDC_CLK, HIGH);
      delayMicroseconds(10);
      digitalWrite(SDC_CLK, LOW);
      current_step -= 1;
      steps_to_go += 1;
      //Serial.println(steps_to_go);
    } 
    // at negative limit
    else {
      steps_to_go = 0;
      //Serial.println("limit reached");
    }
  } 
  // going positive
  else if (steps_to_go > 0) {
    if (!digitalRead(SDC_LIMN)) {
      digitalWrite(SDC_DIR, HIGH);
      delayMicroseconds(10);
      digitalWrite(SDC_CLK, HIGH);
      delayMicroseconds(10);
      digitalWrite(SDC_CLK, LOW);
      current_step += 1;
      steps_to_go -= 1;
      //Serial.println(steps_to_go);
    }
    else {
      steps_to_go = 0;
      //Serial.println("pos lim");
      current_step = 0;
    }
  }
  
}

void setup() {
  // all init funcs
 
  Serial.begin(9600);


  // init piezos
  analogWriteFrequency(piezos[0], 585937.5);
  analogWriteFrequency(piezos[1], 585937.5);
  analogWriteFrequency(piezos[2],585937.5);
  analogWriteFrequency(piezos[3], 585937.5);
  analogWriteFrequency(piezos[4], 585937.5);

  analogWrite(piezos[0],0);
  analogWrite(piezos[1],0);
  analogWrite(piezos[2],0);
  analogWrite(piezos[3],0);
  analogWrite(piezos[4],0);
  //
  //Serial.print("piezo");


  // init SDC
  pinMode(SDC_CLK, OUTPUT);
  pinMode(SDC_DIR, OUTPUT);
  pinMode(SDC_EN, OUTPUT);
  pinMode(SDC_ONOFF, OUTPUT);
  pinMode(SDC_LIMP, INPUT);
  pinMode(SDC_LIMN, INPUT);

  digitalWrite(SDC_CLK, LOW);
  digitalWrite(SDC_DIR, LOW);
  digitalWrite(SDC_EN, HIGH);
  digitalWrite(SDC_ONOFF, HIGH);
  //
  

  // setup wattmeters
  
  while((ina2194.begin() != true)||(ina2193.begin() != true)) {
      delay(2000);
  }
  ina2193.linearCalibrate(ina219Reading3_mA, extMeterReading3_mA);
  ina2194.linearCalibrate(ina219Reading4_mA, extMeterReading4_mA);
  // 
  
  //Serial.print("wattmeters");
  /*
  // setup magnetometer
  Wire.setClock(1000000);
  Wire.begin();
  MMC5883.begin();
  MMC5883.calibrate();
  //
  */
  //Serial.print("magnetometer");
  

  // start timers
  piezoPWM.begin(update_piezos, 2000);
  SDCStepper.begin(step_SDC, period);
  messageChecker.begin(readMessage, 100);
  //
}

void loop() {
  // put your main code here, to run repeatedly:
  PC_Voltage = ina2193.getBusVoltage_mV();
  Motor_Voltage = ina2194.getBusVoltage_mV();

  PC_Current = ina2193.getCurrent_mA();
  Motor_Current = ina2194.getCurrent_mA();
  
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
  
  delay(1000);

}
