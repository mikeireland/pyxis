//This file contains a series of general utility functions which the 
//firmware needs in order to convert between its several relevant datatypes
#ifndef DECODE_H_INCLUDE_GUARD
#define DECODE_H_INCLUDE_GUARD

#include <stdint.h>

//void int32_to_bytes(int32_t, uint8_t *byte0_ptr, uint8_t *byte1_ptr); // for steps
//void uint16_to_bytes(uint16_t, uint8_t *byte0_ptr, uint8_t *byte1_ptr); // for period

int16_t bytes_to_int16(uint8_t byte0, uint8_t byte1);
//Takes in the two bytes representing a the signed integer version of the robot velocity
//and converts it to to a physical velocity as a double precision float
double VelocityBytesToPhysicalDouble(unsigned char byte0, unsigned char byte1);
//Opposite of the above
void PhysicalDoubleToVelocityBytes(double velocity, unsigned char *byte0_ptr, unsigned char *byte1_ptr);
//Takes in the two bytes representing a the signed integer version of an accelerometer reading
//and converts it to to a physical velocity as a double precision float
double AccelerationBytesToPhysicalDouble(unsigned char byte0, unsigned char byte1); 

//Takes in a short integer, and pointers to two bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void ShortIntToBytes(short int integer,unsigned char *byte0_ptr, unsigned char *byte1_ptr);

//Takes in a short integer, and pointers to four bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void IntToBytes(int integer,unsigned char *byte0_ptr, unsigned char *byte1_ptr,unsigned char *byte2_ptr, unsigned char *byte3_ptr); 

//Takes the byte data of an unsigned integer and returns it as an integer
unsigned int BytesTouInt(unsigned char byte0, unsigned char byte1,unsigned char byte2, unsigned char byte3); 

//Takes the byte data of an signed integer and returns it as an integer
int BytesToInt(unsigned char byte0, unsigned char byte1,unsigned char byte2, unsigned char byte3); 

#endif


      
