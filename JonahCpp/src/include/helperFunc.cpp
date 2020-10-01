#include <cmath>
#include <string>

using namespace std;

extern const double kPi = 3.141592654;

double sinc(double x){
    double result = sin(kPi*x)/(kPi*x);
    return result;
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
