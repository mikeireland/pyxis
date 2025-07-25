//This file contains a series of general utility functions which the 
//firmware needs in order to convert between its several relevant datatypes


//Takes in the two bytes representing a signed short integer and converts it to an integer
double VelocityBytesToPhysicalDouble(byte byte0, byte byte1) {
  short int v_temp = (byte0 << 8) | (byte1 << 0);
  double v_double_temp = (double) v_temp;
  double v_real = (v_double_temp/32768.0)*0.015;
  return v_real;
}

//Takes in a short integer, and pointers to two bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void ShortIntToBytes(short int integer,byte *byte0_ptr, byte *byte1_ptr){
  *byte0_ptr = (integer >> 8)&0xFF;
  *byte1_ptr = (integer >> 0)&0xFF;
}

//Takes in a short integer, and pointers to four bytes of a write buffer, and writes the bytes 
//of the integer into the buffers. (works for both signed and unsigned integers
void IntToBytes(int integer,byte *byte0_ptr, byte *byte1_ptr,byte *byte2_ptr, byte *byte3_ptr){
  *byte0_ptr = (integer >> 24)&0xFF;
  *byte1_ptr = (integer >> 16)&0xFF;
  *byte2_ptr = (integer >> 8)&0xFF;
  *byte3_ptr = (integer >> 0)&0xFF;
}



      
