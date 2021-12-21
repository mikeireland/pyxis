// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/file.h> // Contains the flock command

#include "Commands.h"
#include "ErrorCodes.h"

int SetupSerialPort(int port){
    // Create new termios struct, we call it 'tty' for convention
    // No need for "= {0}" at the end as we'll immediately write the existing
    // config to this struct
    struct termios tty;

    // Read in existing settings, and handle any error
    // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
    // must have been initialized with a call to tcgetattr() overwise behaviour
    // is undefined
    if(tcgetattr(port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return -1;
        }

    tty.c_cflag &= ~PARENB; //Clear parity bit
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all the size bits
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common) (not sure about this one, it might be meant to be enabled)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 0;    // Wait for up to 1decisecond, returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, 480*1000000);
    cfsetospeed(&tty, 480*1000000);
    //cfsetispeed(&tty, B460800);
    //cfsetospeed(&tty, B460800);

    //Set the attributes for the port correctly
    if (tcsetattr(port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return -1;
        }

    return 0;
}

//Converts the bytes of a unsigned short int into the value itself
short int TwoBytesToShortInt(unsigned char byte0,unsigned char byte1){
    short int integer = (byte0 << 8) | (byte1 << 0);
    return integer;
}

class RobotComms {
    public:
        RobotComms(){
            //Open communication with the teensy
            teensy = open("/dev/ttyACM0",O_RDWR);

            //Acquire non-blocking exclusive lock
            if(flock(teensy, LOCK_EX | LOCK_NB) == -1) {
                printf("Serial port with file descriptor %d is already locked by another process.",teensy);
            }

            if(teensy<0) {printf("Error %i from open: %s\n", errno, strerror(errno));}
            if(SetupSerialPort(teensy)!=0){printf("Setup Error\n");}
        }

    
        int teensy;
        unsigned char packet[64] = {0x00};
        unsigned char read_buff[64] = {0x00};

    void clear_packet(){
        for(int i = 0; i < 64; ++i){
            packet[i] = 0x00;
        }
    }
    void clear_read_buff(){
        for(int i = 0; i < 64; ++i){
           read_buff[i] = 0x00;
        }
    }


    void read_incoming(){

    int n = read(teensy,&read_buff,sizeof(read_buff));
    unsigned int read_time = 0;
    unsigned int scheduler_time = 0;
    short int read_x_temp = 0;
    short int read_y_temp = 0;
    short int read_z_temp = 0;
    float acc_x = 0;
    float acc_y = 0;
    float acc_z = 0;
    const float gravity_conversion_factor = (9.81/16110.0);

    int i = 0;
    while(i < sizeof(read_buff)){
    switch(read_buff[i]){
        case RUNTIME:
            scheduler_time = (read_buff[i+1] << 24) + (read_buff[i+2] << 16) + (read_buff[i+3] << 8) + (read_buff[i+4] << 0);
            printf("scheduler time was %u\n",scheduler_time);
            i += 5;
            break;
        case Acc0Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 0: x = %f\n",acc_x);
            printf("Accelerometer 0: y = %f\n",acc_y);
            printf("Accelerometer 0: z = %f\n",acc_z);
            i += 7;
            break;
        case Acc1Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 1: x = %f\n",acc_x);
            printf("Accelerometer 1: y = %f\n",acc_y);
            printf("Accelerometer 1: z = %f\n",acc_z);
            i += 7;
            break;
        case Acc2Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 2: x = %f\n",acc_x);
            printf("Accelerometer 2: y = %f\n",acc_y);
            printf("Accelerometer 2: z = %f\n",acc_z);
            i += 7;
            break;
        case Acc3Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 3: x = %f\n",acc_x);
            printf("Accelerometer 3: y = %f\n",acc_y);
            printf("Accelerometer 3: z = %f\n",acc_z);
            i += 7;
            break;
        case Acc4Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 4: x = %f\n",acc_x);
            printf("Accelerometer 4: y = %f\n",acc_y);
            printf("Accelerometer 4: z = %f\n",acc_z);
            i += 7;
            break;
        case Acc5Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            read_y_temp = TwoBytesToShortInt(read_buff[i+3],read_buff[i+4]);
            read_z_temp = TwoBytesToShortInt(read_buff[i+5],read_buff[i+6]);
            acc_x = (double) read_x_temp*gravity_conversion_factor;
            acc_y = (double) read_y_temp*gravity_conversion_factor;
            acc_z = (double) read_z_temp*gravity_conversion_factor;

            printf("Accelerometer 5: x = %f\n",acc_x);
            printf("Accelerometer 5: y = %f\n",acc_y);
            printf("Accelerometer 5: z = %f\n",acc_z);
            i += 7;
            break;
        case Raw0Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 0 in microm/s: v0 = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case Raw1Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 1 in microm/s: v1 = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case Raw2Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 2 in microm/s: v2 = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case Raw3Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 3 in microm/s: v3 = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case Raw4Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 4 in microm/s: v4 = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case Raw5Wr:
            read_x_temp = TwoBytesToShortInt(read_buff[i+1],read_buff[i+2]);
            printf("Velocity of Motor 5 in microm/s: v5  = %d with raw bytes %X %X\n",read_x_temp,read_buff[i+1],read_buff[i+2]);
            i += 3;
            break;
        case 0x00:
            if(read_buff[i+1]==0x00){if(read_buff[i+2]==0x00){
                i = 64;//If we have reached three consecutive nulls, end the loop
                break;
                }}
            i += 1;
            break;
        case UnrecVal:
            printf("Unrecognised Command\n");
            i += 1;
            break;
        case SchedFull:
            printf("Schedule Full\n");
            i += 1;
            break;
        case PackFail:
            printf("Packet Start Not Present\n");
            i += 1;
            break;
        default:
            printf("Unknown Code\n");
            i += 1;
            break;
    }} 
    }

    void RobotMain(){
        //Lets try writing a small velocity for a few seconds and then reading it back
    short int v_target = 15000;
    clear_packet();
    packet[0] = 0xFF;
    packet[1] = SetBFFy;
    packet[2] = (v_target >> 8)&0xFF;
    packet[3] = (v_target >> 0)&0xFF;
    write(teensy,packet,sizeof(packet));

    sleep(1);

    for(int j = 0; j < 10000; ++j){
    //Our packet will have two commands: Write a new velocity: Request the packet read time
    clear_packet();
    //All packets start with a 0xFF
    packet[0] = 0xFF;

    packet[1] = Acc0Wr; //We request the accelerations of once of the accelerometers
    packet[2] = EMPTY;
    packet[3] = EMPTY;

    packet[4] = Acc1Wr; //We request the accelerations of once of the accelerometers
    packet[5] = EMPTY;
    packet[6] = EMPTY;

    packet[7] = Acc2Wr; //We request the accelerations of once of the accelerometers
    packet[8] = EMPTY;
    packet[9] = EMPTY;

    packet[10] = Acc3Wr; //We request the accelerations of once of the accelerometers
    packet[11] = EMPTY;
    packet[12] = EMPTY;

    packet[13] = Acc4Wr; //We request the accelerations of once of the accelerometers
    packet[14] = EMPTY;
    packet[15] = EMPTY;

    packet[16] = Acc5Wr; //We request the accelerations of once of the accelerometers
    packet[17] = EMPTY;
    packet[18] = EMPTY;

    packet[19] = Raw0Wr; //We request the velocity of one of the motors
    packet[20] = EMPTY;
    packet[21] = EMPTY;

    packet[22] = Raw1Wr; //We request the velocity of one of the motors
    packet[23] = EMPTY;
    packet[24] = EMPTY;

    packet[25] = Raw2Wr; //We request the velocity of one of the motors
    packet[26] = EMPTY;
    packet[27] = EMPTY;

    packet[28] = Raw3Wr; //We request the velocity of one of the motors
    packet[29] = EMPTY;
    packet[30] = EMPTY;

    packet[31] = Raw4Wr; //We request the velocity of one of the motors
    packet[32] = EMPTY;
    packet[33] = EMPTY;

    packet[34] = Raw5Wr; //We request the velocity of one of the motors
    packet[35] = EMPTY;
    packet[36] = EMPTY;

    packet[37] = RUNTIME;
    packet[38] = EMPTY;
    packet[39] = EMPTY;

    packet[40] = Acc0Wr; //We request the accelerations of once of the accelerometers
    packet[41] = EMPTY;
    packet[42] = EMPTY;

    packet[43] = Acc1Wr; //We request the accelerations of once of the accelerometers
    packet[44] = EMPTY;
    packet[45] = EMPTY;

    packet[46] = Acc2Wr; //We request the accelerations of once of the accelerometers
    packet[47] = EMPTY;
    packet[48] = EMPTY;

    packet[49] = Acc3Wr; //We request the accelerations of once of the accelerometers
    packet[50] = EMPTY;
    packet[51] = EMPTY;

    packet[52] = Acc4Wr; //We request the accelerations of once of the accelerometers
    packet[53] = EMPTY;
    packet[54] = EMPTY;

    packet[55] = RUNTIME;
    packet[56] = EMPTY;
    packet[57] = EMPTY;

    packet[58] = RUNTIME;
    packet[59] = EMPTY;
    packet[60] = EMPTY;

    packet[61] = EMPTY;
    packet[62] = EMPTY;
    packet[63] = EMPTY;

    write(teensy, packet, sizeof(packet));
    printf("run number %d\n",j);

    //We are requesting two packets worth of data so we read off two packets
    clear_read_buff();
    read_incoming();
    clear_read_buff();
    read_incoming();
    usleep(1000);
    
    }
    v_target = -15000;
    clear_packet();
    packet[0] = 0xFF;
    packet[1] = SetBFFy;
    packet[2] = (v_target >> 8)&0xFF;
    packet[3] = (v_target >> 0)&0xFF;
    write(teensy,packet,sizeof(packet));

    sleep(1);

    for(int j = 0; j < 10000; ++j){
    //Our packet will have two commands: Write a new velocity: Request the packet read time
    clear_packet();
    //All packets start with a 0xFF
    packet[0] = 0xFF;

    packet[1] = Acc0Wr; //We request the accelerations of once of the accelerometers
    packet[2] = EMPTY;
    packet[3] = EMPTY;

    packet[4] = Acc1Wr; //We request the accelerations of once of the accelerometers
    packet[5] = EMPTY;
    packet[6] = EMPTY;

    packet[7] = Acc2Wr; //We request the accelerations of once of the accelerometers
    packet[8] = EMPTY;
    packet[9] = EMPTY;

    packet[10] = Acc3Wr; //We request the accelerations of once of the accelerometers
    packet[11] = EMPTY;
    packet[12] = EMPTY;

    packet[13] = Acc4Wr; //We request the accelerations of once of the accelerometers
    packet[14] = EMPTY;
    packet[15] = EMPTY;

    packet[16] = Acc5Wr; //We request the accelerations of once of the accelerometers
    packet[17] = EMPTY;
    packet[18] = EMPTY;

    packet[19] = Raw0Wr; //We request the velocity of one of the motors
    packet[20] = EMPTY;
    packet[21] = EMPTY;

    packet[22] = Raw1Wr; //We request the velocity of one of the motors
    packet[23] = EMPTY;
    packet[24] = EMPTY;

    packet[25] = Raw2Wr; //We request the velocity of one of the motors
    packet[26] = EMPTY;
    packet[27] = EMPTY;

    packet[28] = Raw3Wr; //We request the velocity of one of the motors
    packet[29] = EMPTY;
    packet[30] = EMPTY;

    packet[31] = Raw4Wr; //We request the velocity of one of the motors
    packet[32] = EMPTY;
    packet[33] = EMPTY;

    packet[34] = Raw5Wr; //We request the velocity of one of the motors
    packet[35] = EMPTY;
    packet[36] = EMPTY;

    packet[37] = RUNTIME;
    packet[38] = EMPTY;
    packet[39] = EMPTY;

    packet[40] = Acc0Wr; //We request the accelerations of once of the accelerometers
    packet[41] = EMPTY;
    packet[42] = EMPTY;

    packet[43] = Acc1Wr; //We request the accelerations of once of the accelerometers
    packet[44] = EMPTY;
    packet[45] = EMPTY;

    packet[46] = Acc2Wr; //We request the accelerations of once of the accelerometers
    packet[47] = EMPTY;
    packet[48] = EMPTY;

    packet[49] = Acc3Wr; //We request the accelerations of once of the accelerometers
    packet[50] = EMPTY;
    packet[51] = EMPTY;

    packet[52] = Acc4Wr; //We request the accelerations of once of the accelerometers
    packet[53] = EMPTY;
    packet[54] = EMPTY;

    packet[55] = RUNTIME;
    packet[56] = EMPTY;
    packet[57] = EMPTY;

    packet[58] = RUNTIME;
    packet[59] = EMPTY;
    packet[60] = EMPTY;

    packet[61] = EMPTY;
    packet[62] = EMPTY;
    packet[63] = EMPTY;

    write(teensy, packet, sizeof(packet));
    printf("run number %d\n",j);

    //We are requesting two packets worth of data so we read off two packets
    clear_read_buff();
    read_incoming();
    clear_read_buff();
    read_incoming();

    usleep(1000);
    }

    v_target = 0;

    clear_packet();

    packet[0] = 0xFF;

    packet[1] = SetBFFy;
    packet[2] = (v_target >> 8)&0xFF;
    packet[3] = (v_target >> 0)&0xFF;   
    
    packet[4] = Raw0Wr; //We request the accelerations of once of the accelerometers
    packet[5] = EMPTY;
    packet[6] = EMPTY;

    packet[7] = Raw1Wr; //We request the accelerations of once of the accelerometers
    packet[8] = EMPTY;
    packet[9] = EMPTY;

    packet[10] = Raw2Wr; //We request the accelerations of once of the accelerometers
    packet[11] = EMPTY;
    packet[12] = EMPTY;

    packet[13] = Raw3Wr; //We request the accelerations of once of the accelerometers
    packet[14] = EMPTY;
    packet[15] = EMPTY;

    packet[16] = Raw4Wr; //We request the accelerations of once of the accelerometers
    packet[17] = EMPTY;
    packet[18] = EMPTY;

    packet[19] = Raw5Wr; //We request the accelerations of once of the accelerometers
    packet[20] = EMPTY;
    packet[21] = EMPTY; 

    packet[22] = RUNTIME;
    packet[23] = EMPTY;
    packet[24] = EMPTY; 

    write(teensy,packet,sizeof(packet));

    clear_read_buff();
    read_incoming();   
    
    for(int j = 0; j<1000; ++j){
    clear_packet();

    packet[0] = 0xFF;
    
    packet[1] = Raw0Wr; //We request the accelerations of once of the accelerometers
    packet[2] = EMPTY;
    packet[3] = EMPTY;

    packet[4] = Raw1Wr; //We request the accelerations of once of the accelerometers
    packet[5] = EMPTY;
    packet[6] = EMPTY;

    packet[7] = Raw2Wr; //We request the accelerations of once of the accelerometers
    packet[8] = EMPTY;
    packet[9] = EMPTY;

    packet[10] = Raw3Wr; //We request the accelerations of once of the accelerometers
    packet[11] = EMPTY;
    packet[12] = EMPTY;

    packet[13] = Raw4Wr; //We request the accelerations of once of the accelerometers
    packet[14] = EMPTY;
    packet[15] = EMPTY;

    packet[16] = Raw5Wr; //We request the accelerations of once of the accelerometers
    packet[17] = EMPTY;
    packet[18] = EMPTY; 

    write(teensy,packet,sizeof(packet));

    clear_read_buff();
    read_incoming();   
    usleep(10000);
    
    }

    usleep(100000);
    
    close(teensy);
    }
};

int main(){
    RobotComms comms;
    comms.RobotMain();    
    printf("Success\n");
    return 0;
}