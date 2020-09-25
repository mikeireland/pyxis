// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _HELPERFUNC_
#define _HELPERFUNC_


/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num = 25, const char padding_char = ' ');


#endif // _HELPERFUNC_
