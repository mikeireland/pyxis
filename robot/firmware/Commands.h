//This file lists names of controller commands
//and the byte assigned to each name

#define EMPTY 0x00 //The empty command
#define RUNTIME 0x01 //Request the longest runtime of the scheduler
#define STOP 0x02 //The command to all stop and reset the schedule
//TODO Add commands to request the read and write times for packets

//Update the raw motor velocities (for closing the levelling servo loop rigorously)
#define SetRaw0 0x10 //Motor 0
#define SetRaw1 0x11 //Motor 1
#define SetRaw2 0x12 //Motor 2
#define SetRaw3 0x13 //Actuator 0
#define SetRaw4 0x14 //Actuator 1
#define SetRaw5 0x15 //Actuator 2

//report motor_vels values
#define Raw0Wr 0x20
#define Raw1Wr 0x21
#define Raw2Wr 0x22
#define Raw3Wr 0x23
#define Raw4Wr 0x24
#define Raw5Wr 0x25

//Read accelerometer axes
#define Acc0ReX 0xA0
#define Acc0ReY 0xA1
#define Acc0ReZ 0xA2

//Read accelerometer axes
#define Acc1ReX 0xB0
#define Acc1ReY 0xB1
#define Acc1ReZ 0xB2

//Read accelerometer axes
#define Acc2ReX 0xC0
#define Acc2ReY 0xC1
#define Acc2ReZ 0xC2

//Read accelerometer axes
#define Acc3ReX 0xD0
#define Acc3ReY 0xD1
#define Acc3ReZ 0xD2

//Read accelerometer axes
#define Acc4ReX 0xE0
#define Acc4ReY 0xE1
#define Acc4ReZ 0xE2

//Read accelerometer axes
#define Acc5ReX 0xF0
#define Acc5ReY 0xF1
#define Acc5ReZ 0xF2

//Write each accelerometer axis
#define Acc0Wr 0xA3
#define Acc1Wr 0xB3
#define Acc2Wr 0xC3
#define Acc3Wr 0xD3
#define Acc4Wr 0xE3
#define Acc5Wr 0xF3
