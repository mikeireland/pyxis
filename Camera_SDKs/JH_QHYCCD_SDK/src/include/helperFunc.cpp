#include <cmath>
#include <string>

using namespace std;

extern const double kPi = 3.141592654;

double sinc(double x){
    if (x == 0){
        return 1.0;
    } else{
    double result = sin(kPi*x)/(kPi*x);
    return result;
}
}

/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num, const char padding_char) {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), padding_char);
        }
        return str + ": ";
    }
    
unsigned short combine_chars(unsigned char a, unsigned char b){
	unsigned short x = 256*a + b;
	return x;
}

void char_to_short(unsigned char* input, unsigned short* output, unsigned long length){
	for (unsigned int i=0; i<length; i=i+2){
		output[i/2] = combine_chars(input[i],input[i+1]);
	}
	return;
}
