// This is a test program that puts both accelerometer and moving together. 
#include <SPI.h>
//*** Defines which should be changed ***
//The number of microseconds in-between microstep. 
//#define USEC  125 //125 slew speed @ 16 microsteps. 37500 max tracking speed.
//#define USEC  2000 //125 slew speed @ 16 microsteps. 37500 max tracking speed.
#define USEC  8 //125 slew speed @ 16 microsteps. 37500 max tracking speed.

//*** Constants for the Goniometer ***
#define STEP  0
#define DIR   1
#define LIMP  8
#define LIMN  9
#define LIMZ  7

//*** Constants for the Accelerometer ***
#define DEVID_AD                 0x00
#define DEVID_MST                0x01
#define PARTID                   0x02
#define REVID                    0x03
#define STATUS                   0x04
#define FIFO_ENTRIES             0x05
#define TEMP2 0x06
#define TEMP1 0x07
const int XDATA3 = 0x08;
const int XDATA2 = 0x09;
const int XDATA1 = 0x0A;
const int YDATA3 = 0x0B;
const int YDATA2 = 0x0C;
const int YDATA1 = 0x0D;
const int ZDATA3 = 0x0E;
const int ZDATA2 = 0x0F;
const int ZDATA1 = 0x10;
const int RANGE = 0x2C;
const int POWER_CTL = 0x2D;

#define AXIS ZDATA3

/* Temperature parameters */
#define ADXL355_TEMP_BIAS       (float)1852.0      /* Accelerometer temperature bias(in ADC codes) at 25 Deg C */
#define ADXL355_TEMP_SLOPE      (float)-9.05       /* Accelerometer temperature change from datasheet (LSB/degC) */
#define LSB_G (float)256000.0
// Device values
const int RANGE_2G = 0x01;
const int RANGE_4G = 0x02;
const int RANGE_8G = 0x03;
const int MEASURE_MODE = 0x04; // Only accelerometer is 0x06 with data ready off. 

// Operations
const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;

// Pins used for the connection with the sensor
const int CHIP_SELECT_PIN = 10;

//A sequence of inputs
int inInts[9];
int assembledInt;
short inShorts[8];
short assembledShort;
float f32temp = 0.0f;
int inByte=0;

void setup() {
  // Initalize the chip select pins for the accelerometer
  pinMode(CHIP_SELECT_PIN, OUTPUT);
  
  // Set up Accelerometer
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  SPI.begin();

  // Initalize the  data ready and chip select pins:
  pinMode(CHIP_SELECT_PIN, OUTPUT);

  //Configure ADXL355:
  writeRegister(RANGE, RANGE_2G); 
  writeRegister(POWER_CTL, MEASURE_MODE); // Enable measure mode
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  } 
  
  // Set up Motor
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(LIMP, INPUT_PULLUP);
  pinMode(LIMN, INPUT_PULLUP);
  pinMode(LIMZ, INPUT_PULLUP);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check standard accelerometer settings and communication
  inByte = readRegister(DEVID_AD);
  Serial.print("DEVID_AD: ");
  Serial.println(inByte, HEX);
  inByte = readRegister(DEVID_MST);
  Serial.print("DEVID_MST: ");
  Serial.println(inByte, HEX);
  inByte = readRegister(PARTID);
  Serial.print("PARTID: ");
  Serial.println(inByte, HEX);
  delay(10);
  
  // Temperature, read in the same way as the accelerations:
  readSequence(TEMP2, 2, inInts) ;
  assembledInt = (inInts[0]<<8) + inInts[1];
  Serial.print("TEMP int: ");
  Serial.println(assembledInt);
  f32temp = ((((float)assembledInt - ADXL355_TEMP_BIAS)) / ADXL355_TEMP_SLOPE) + 25.0;
  Serial.print("TEMP (C): ");
  Serial.println(f32temp);

  // Now wait for User input
  Serial.println("Enter command:");
  Serial.println("f (forward), b (back), s (small forward), r (small back)");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    if (inByte=='f'){
      digitalWrite(DIR, HIGH);
      move_motor(3200, false);
    } else if (inByte=='b'){
      digitalWrite(DIR, LOW);
      move_motor(3200, false);
    } else if (inByte=='F'){
      digitalWrite(DIR, HIGH);
      move_motor(320000, false);
    } else if (inByte=='B'){ //small step forward
      digitalWrite(DIR, LOW);
      move_motor(320000, false);
    } else if (inByte=='s'){ //small step forward
      digitalWrite(DIR, HIGH);
      move_motor(320, true);
    } else if (inByte=='r'){ // small step reverse
      digitalWrite(DIR, LOW);
      move_motor(320, true);
    } else {
      Serial.print("Unknown Character: ");
      Serial.println(inByte, DEC);
    }
  }
}

//**** Routines ***

// The main motor movement routine. During the movement, time taken for reads and writes is recorded, and displayed 
// at the end.
void move_motor(int nsteps, bool ignore_limits){
  unsigned long usec_then, usec_now;
  int dt_integral=0;
  usec_then = micros();
  if (!digitalRead(LIMP)) Serial.println("Hit positive limit (start of move)");
  if (!digitalRead(LIMN)) Serial.println("Hit negative limit (start of move)");
  if (!digitalRead(LIMZ)) Serial.println("Hit zero (start of move)");
  usec_now = micros();
  Serial.print("About to move. Limit reading time (usec): ");
  Serial.println(usec_now-usec_then, DEC);
  for (int i=0;i<nsteps;i++){ //3200 one revolution
    if ((digitalRead(LIMP) && digitalRead(LIMN) && digitalRead(LIMZ)) || ignore_limits){
      digitalWrite(STEP, HIGH);
      //Wait for tick
      //all_axes_acceleration();
      delayMicroseconds(1);
      while ((micros() % USEC) != 0){ /* Delay */}
      //delayMicroseconds(USEC); 
      digitalWrite(STEP, LOW);
      //Get acceleration of desired axis and see how long this takes.
      usec_then = micros();
      //all_axes_acceleration();
      dt_integral += micros()-usec_then;
      delayMicroseconds(1);
      while ((micros() % USEC) != 0){/* Delay */}
    } 
  }
  Serial.print("Average time for reading acceleration (usec): ");
  Serial.println((float)dt_integral/(float)nsteps);
  if (!digitalRead(LIMP)) Serial.println("Hit positive limit (end of move)");
  if (!digitalRead(LIMN)) Serial.println("Hit negative limit (end of move)");
  if (!digitalRead(LIMZ)) Serial.println("Hit zero (end of move)");
}

// Read one axis acceleration and print.
void one_axis_acceleration(int axis){
  readShortSequence(axis, 2, inShorts) ;
  assembledShort = (inShorts[0]<<8) + inShorts[1];
  Serial.println(assembledInt);
}

// Read one axis acceleration and print.
void all_axes_acceleration(){
  readShortSequence(XDATA3, 8, inShorts) ;
  assembledShort = (inShorts[0]<<8) + inShorts[1];
  Serial.print(assembledShort);
  Serial.print(" ");
  assembledShort = (inShorts[3]<<8) + inShorts[4];
  Serial.print(assembledShort);
  Serial.print(" ");
  assembledShort = (inShorts[6]<<8) + inShorts[7];
  Serial.println(assembledShort);
}
/* 
 * Write registry in specific device address
 */
void writeRegister(byte thisRegister, byte thisValue) {
  byte dataToSend = (thisRegister << 1) | WRITE_BYTE;
  digitalWrite(CHIP_SELECT_PIN, LOW);
  SPI.transfer(dataToSend);
  SPI.transfer(thisValue);
  digitalWrite(CHIP_SELECT_PIN, HIGH);
}

/* 
 * Read registry in specific device address
 */
unsigned int readRegister(byte thisRegister) {
  unsigned int result = 0;
  byte dataToSend = (thisRegister << 1) | READ_BYTE;

  digitalWrite(CHIP_SELECT_PIN, LOW);
  SPI.transfer(dataToSend);
  result = SPI.transfer(0x00);
  digitalWrite(CHIP_SELECT_PIN, HIGH);
  return result;
}

/* 
 * Read multiple registries
 */
void readMultipleData(int *addresses, int dataSize, int *readedData) {
  digitalWrite(CHIP_SELECT_PIN, LOW);
  for(int i = 0; i < dataSize; i = i + 1) {
    byte dataToSend = (addresses[i] << 1) | READ_BYTE;
    SPI.transfer(dataToSend);
    readedData[i] = SPI.transfer(0x00);
  }
  digitalWrite(CHIP_SELECT_PIN, HIGH);
}

/* 
 * Read multiple registries
 */
void readSequence(byte thisRegister, int dataSize, int *dataRead) {
  digitalWrite(CHIP_SELECT_PIN, LOW);
  byte dataToSend = (thisRegister << 1) | READ_BYTE;
  SPI.transfer(dataToSend);  
  for(int i = 0; i < dataSize; i = i + 1) {
    dataRead[i] = SPI.transfer(0x00);
  }
  digitalWrite(CHIP_SELECT_PIN, HIGH);
}

/* 
 * Read multiple registries
 */
void readShortSequence(byte thisRegister, int dataSize, short *dataRead) {
  digitalWrite(CHIP_SELECT_PIN, LOW);
  byte dataToSend = (thisRegister << 1) | READ_BYTE;
  SPI.transfer(dataToSend);  
  for(int i = 0; i < dataSize; i = i + 1) {
    dataRead[i] = SPI.transfer(0x00);
  }
  digitalWrite(CHIP_SELECT_PIN, HIGH);
}
