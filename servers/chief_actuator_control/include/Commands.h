#ifndef COMMANDS_H_INCLUDE_GUARD
#define COMMANDS_H_INCLUDE_GUARD
//This file lists names of controller commands
//and the byte assigned to each name

#define START 0xFF
#define EMPTY 0x00 //The empty command
#define TID 0x04
#define WATTMETER 0x21
#define COMPASS 0x22
#define SETPWM 0x31 // requires 5x uint8_t args for duty cycles
#define GETPWM 0x32 
#define SETSDC 0x33 // int32_t steps, uint16_t period
#define GETSDC 0x34

#endif
