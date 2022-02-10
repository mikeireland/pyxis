#ifndef ERROR_CODES_H_INCLUDE_GUARD
#define ERROR_CODES_H_INCLUDE_GUARD
//This file includes a series of macros which are reported
//by the device back to the host as error codes.

#define UnrecVal 0xFD //If the code of a command is unrecognised
#define SchedFull 0xFE //If the schedule is full
#define PackFail 0xFF //If a received packet does not begin correctly

#endif