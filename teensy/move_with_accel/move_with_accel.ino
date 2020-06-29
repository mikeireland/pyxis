// This is a test program that puts both accelerometer and moving together. 
#include <SPI.h>
//*** Constants which should be changed ***
//The number of microseconds in-between microstep. 
//#define kDeltaT_us  125 //125 slew speed @ 16 microsteps. 37500 max tracking speed.
//#define kDeltaT_us  2000 //125 slew speed @ 16 microsteps. 37500 max tracking speed.
const int kDeltaT_us=8; //Half of slew speed @ 256 microsteps.
#define AXIS kZData3Reg

//*** Constants for the Goniometer ***
const int kStepPin=0;
const int kDirPin=1;
const int kLimPPin=8;
const int kLimNPin=9;
const int kLimZPin=7;

//*** Constants for the Accelerometer Registers***
const int kDevidADReg=0x00;
const int kDevidMSTReg=0x01;
const int kPartIDReg=0x02;
const int kTemp2Reg=0x06;
const int kXData3Reg = 0x08;
const int kYData3Reg = 0x0B;
const int kZData3Reg = 0x0E;
const int kRangeReg = 0x2C;
const int kPowerCtlReg = 0x2D;

/* Temperature parameters */
const float kADXL355TempBias=1852.0;      /* Accelerometer temperature bias(in ADC codes) at 25 Deg C */
const float kADXL355TempSlope=-9.05;       /* Accelerometer temperature change from datasheet (LSB/degC) */
const float kLSB_G=256000.0;  /* Least significant bit per g. */

// Device values. NB These should also be changed to k prefixed constants. Or if hardwired, commented 
// properly.
const int RANGE_2G = 0x01;
const int RANGE_4G = 0x02;
const int RANGE_8G = 0x03;
const int MEASURE_MODE = 0x04; // Only accelerometer is 0x06 with data ready off. 

// Operations
const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;

// Pins used for the connection with the sensor
const int kChipSelectPin = 10;

//A sequence of inputs. These are Global variables so should probably 
//be avoided, and instead be local inside either/both of the next two
//routines, with the exception of the OuputAcceleration config.
int inInts[9];
int assembledInt;
short inShorts[8];
short assembledShort;
float f32temp = 0.0f;
int inByte=0;
bool OutputAcceleration=true;

void setup() {
  // Initalize the chip select pins for the accelerometer
  pinMode(kChipSelectPin, OUTPUT);
  
  // Set up Accelerometer
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  SPI.begin();

  // Initalize the  data ready and chip select pins:
  pinMode(kChipSelectPin, OUTPUT);

  //Configure ADXL355:
  writeRegister(kRangeReg, RANGE_2G); 
  writeRegister(kPowerCtlReg, MEASURE_MODE); // Enable measure mode
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  } 
  
  // Set up Motor
  pinMode(kDirPin, OUTPUT);
  pinMode(kStepPin, OUTPUT);
  pinMode(kLimPPin, INPUT_PULLUP);
  pinMode(kLimNPin, INPUT_PULLUP);
  pinMode(kLimZPin, INPUT_PULLUP);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check standard accelerometer settings and communication
  inByte = readRegister(kDevidADReg);
  Serial.print("DEVID_AD: ");
  Serial.println(inByte, HEX);
  inByte = readRegister(kDevidMSTReg);
  Serial.print("DEVID_MST: ");
  Serial.println(inByte, HEX);
  inByte = readRegister(kPartIDReg);
  Serial.print("PARTID: ");
  Serial.println(inByte, HEX);
  delay(10);
  
  // Temperature, read in the same way as the accelerations:
  readSequence(kTemp2Reg, 2, inInts) ;
  assembledInt = (inInts[0]<<8) + inInts[1];
  Serial.print("TEMP int: ");
  Serial.println(assembledInt);
  f32temp = ((((float)assembledInt - kADXL355TempBias)) / kADXL355TempSlope) + 25.0;
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
      digitalWrite(kDirPin, HIGH);
      move_motor(3200, false);
    } else if (inByte=='b'){
      digitalWrite(kDirPin, LOW);
      move_motor(3200, false);
    } else if (inByte=='F'){
      digitalWrite(kDirPin, HIGH);
      move_motor(320000, false);
    } else if (inByte=='B'){ //small step forward
      digitalWrite(kDirPin, LOW);
      move_motor(320000, false);
    } else if (inByte=='s'){ //small step forward
      digitalWrite(kDirPin, HIGH);
      move_motor(320, true);
    } else if (inByte=='r'){ // small step reverse
      digitalWrite(kDirPin, LOW);
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
  if (!digitalRead(kLimPPin)) Serial.println("Hit positive limit (start of move)");
  if (!digitalRead(kLimNPin)) Serial.println("Hit negative limit (start of move)");
  if (!digitalRead(kLimZPin)) Serial.println("Hit zero (start of move)");
  usec_now = micros();
  Serial.print("About to move. Limit reading time (usec): ");
  Serial.println(usec_now-usec_then, DEC);
  for (int i=0;i<nsteps;i++){ //3200 one revolution
    if ((digitalRead(kLimPPin) && digitalRead(kLimNPin) && digitalRead(kLimZPin)) || ignore_limits){
      digitalWrite(kStepPin, HIGH);
      //Wait for tick
      if (OutputAcceleration) all_axes_acceleration();
      delayMicroseconds(1);
      while ((micros() % kDeltaT_us) != 0){ /* Delay */}
      digitalWrite(kStepPin, LOW);
      //Get acceleration of desired axis and see how long this takes.
      usec_then = micros();
      if (OutputAcceleration) all_axes_acceleration();
      dt_integral += micros()-usec_then;
      delayMicroseconds(1);
      while ((micros() % kDeltaT_us) != 0){/* Delay */}
    } 
  }
  Serial.print("Average time for reading acceleration (usec): ");
  Serial.println((float)dt_integral/(float)nsteps);
  if (!digitalRead(kLimPPin)) Serial.println("Hit positive limit (end of move)");
  if (!digitalRead(kLimNPin)) Serial.println("Hit negative limit (end of move)");
  if (!digitalRead(kLimZPin)) Serial.println("Hit zero (end of move)");
}

// Read one axis acceleration and print.
void one_axis_acceleration(int axis){
  readShortSequence(axis, 2, inShorts) ;
  assembledShort = (inShorts[0]<<8) + inShorts[1];
  Serial.println(assembledInt);
}

// Read one axis acceleration and print.
void all_axes_acceleration(){
  readShortSequence(kXData3Reg, 8, inShorts) ;
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
  digitalWrite(kChipSelectPin, LOW);
  SPI.transfer(dataToSend);
  SPI.transfer(thisValue);
  digitalWrite(kChipSelectPin, HIGH);
}

/* 
 * Read registry in specific device address
 */
unsigned int readRegister(byte thisRegister) {
  unsigned int result = 0;
  byte dataToSend = (thisRegister << 1) | READ_BYTE;

  digitalWrite(kChipSelectPin, LOW);
  SPI.transfer(dataToSend);
  result = SPI.transfer(0x00);
  digitalWrite(kChipSelectPin, HIGH);
  return result;
}


/* 
 * Read multiple registers in a row into an array of ints.
 */
void readSequence(byte thisRegister, int dataSize, int *dataRead) {
  digitalWrite(kChipSelectPin, LOW);
  byte dataToSend = (thisRegister << 1) | READ_BYTE;
  SPI.transfer(dataToSend);  
  for(int i = 0; i < dataSize; i = i + 1) {
    dataRead[i] = SPI.transfer(0x00);
  }
  digitalWrite(kChipSelectPin, HIGH);
}

/* 
 * Read multiple registries in a row into an array of shorts.
 */
void readShortSequence(byte thisRegister, int dataSize, short *dataRead) {
  digitalWrite(kChipSelectPin, LOW);
  byte dataToSend = (thisRegister << 1) | READ_BYTE;
  SPI.transfer(dataToSend);  
  for(int i = 0; i < dataSize; i = i + 1) {
    dataRead[i] = SPI.transfer(0x00);
  }
  digitalWrite(kChipSelectPin, HIGH);
}
