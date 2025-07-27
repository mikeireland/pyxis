#include "Decode.h"

//Takes in the two bytes representing a the signed integer version of the robot velocity
//and converts it to to a physical velocity in SI Uniges (m/s) as a double precision float
double VelocityBytesToPhysicalDouble(unsigned char byte0, unsigned char byte1) {
  short int v_temp = (byte0 << 8) | (byte1 << 0);
  double v_double_temp = (double) v_temp;
  double v_real = (v_double_temp/32768.0)*0.015;
  return v_real;
}

//Opposite of the above
void PhysicalDoubleToVelocityBytes(double velocity, unsigned char *byte0_ptr, unsigned char *byte1_ptr) {
  short int v_temp = (short int) ((velocity*32677.0)/0.015);
  *byte0_ptr = (v_temp >> 8)&0xFF;
  *byte1_ptr = (v_temp >> 0)&0xFF;
}

//Takes in the two bytes representing a the signed integer version of an accelerometer reading
//and converts it to to a physical velocity as a double precision float
double AccelerationBytesToPhysicalDouble(unsigned char byte0, unsigned char byte1) {
  short int a_temp = (byte0 << 8) | (byte1 << 0);
  double a_double_temp = (double) a_temp;
  double a_real = a_double_temp*(9.81/16110.0); //(9.81/16110.0) is the conversion factor. Must check this
  return a_real;
}

//Takes in a short integer, and pointers to two bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void ShortIntToBytes(short int integer,unsigned char *byte0_ptr, unsigned char *byte1_ptr){
  *byte0_ptr = (integer >> 8)&0xFF;
  *byte1_ptr = (integer >> 0)&0xFF;
}

//Takes in a short integer, and pointers to four bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void IntToBytes(int integer,unsigned char *byte0_ptr, unsigned char *byte1_ptr,unsigned char *byte2_ptr, unsigned char *byte3_ptr) {
  *byte0_ptr = (integer >> 24)&0xFF;
  *byte1_ptr = (integer >> 16)&0xFF;
  *byte2_ptr = (integer >> 8)&0xFF;
  *byte3_ptr = (integer >> 0)&0xFF;
}

//Takes the byte data of an unsigned integer and returns it as an integer
unsigned int BytesTouInt(unsigned char byte0, unsigned char byte1,unsigned char byte2, unsigned char byte3) {
  unsigned int integer = (byte0 << 24) + (byte1 << 16) + (byte2 << 8) + (byte3 << 0);
  return integer;
}

//Takes the byte data of an signed integer and returns it as an integer
int BytesToInt(unsigned char byte0, unsigned char byte1,unsigned char byte2, unsigned char byte3) {
  int integer = (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | (byte3 << 0);
  return integer;
}
