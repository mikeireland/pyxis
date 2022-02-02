//SerialPort.cpp
#include "SerialPort.h"

// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/file.h> // Contains the flock command

//Macro headers
#include "Commands.h"
#include "ErrorCodes.h"

using namespace Comms;

int SerialPort::max_packet_size_ = 64; 

//Class constructor to sets up the serial connection to the teensy correctly
//We also zero all of the input registers
SerialPort::SerialPort() {
    teensy_ = open("/dev/ttyACM0",O_RDWR);
    //Acquire non-blocking exclusive lock
    /*if(flock(teensy_, LOCK_EX | LOCK_NB) == -1) {
    printf("Serial port with file descriptor %d is already locked by another process.",teensy_);
    }*/

    if(teensy_ < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    // Read in existing settings, and handle any error
    // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
    // must have been initialized with a call to tcgetattr() overwise behaviour
    // is undefined
    struct termios tty;
    if(tcgetattr(teensy_, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
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

    tty.c_cc[VTIME] = 0;    //Return as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, 480*1000000); //Set the speeds to the maxmium USB2.0 speed
    cfsetospeed(&tty, 480*1000000);

    //Set the attributes for the port correctly
    if (tcsetattr(teensy_, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        }
    printf("Comms Opened\n");

    //Zeroing registers and buffers
    ClearReadBuff();
    ClearWriteBuff();
    ClearStructVelBytes(&translational_velocities_in_);
    ClearStructVelBytes(&actuator_velocities_in_);
    ClearStructVelBytes(&actuator_velocities_out_);
    ClearStructVelBytes(&motor_velocities_out_);
    ClearStructAccelBytes(&accelerometer0_in_);
    ClearStructAccelBytes(&accelerometer1_in_);
    ClearStructAccelBytes(&accelerometer2_in_);
    ClearStructAccelBytes(&accelerometer3_in_);
    ClearStructAccelBytes(&accelerometer4_in_);
    ClearStructAccelBytes(&accelerometer5_in_);
    runtime_in_[0] = 0x00;
    runtime_in_[1] = 0x00;
    runtime_in_[2] = 0x00;
    runtime_in_[3] = 0x00;
}

//Closes the communication with the teensy
void SerialPort::ClosePort() {
    sleep(1);
    close(teensy_);
    printf("Comms Closed\n");
}

void SerialPort::WriteMessage() {
    write(teensy_,write_buffer_,sizeof(write_buffer_));
    ClearWriteBuff();
}

void SerialPort::ReadMessage() {
    ClearReadBuff();
    read(teensy_,&read_buffer_,sizeof(read_buffer_));

    int i = 0;
    while(i < sizeof(read_buffer_)){
    switch(read_buffer_[i]){
        case RUNTIME:
            runtime_in_[0] = read_buffer_[i+1];
            runtime_in_[1] = read_buffer_[i+2];
            runtime_in_[2] = read_buffer_[i+3];
            runtime_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step0Wr:
            step_count0_in_[0] = read_buffer_[i+1];
            step_count0_in_[1] = read_buffer_[i+2];
            step_count0_in_[2] = read_buffer_[i+3];
            step_count0_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step1Wr:
            step_count1_in_[0] = read_buffer_[i+1];
            step_count1_in_[1] = read_buffer_[i+2];
            step_count1_in_[2] = read_buffer_[i+3];
            step_count1_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step2Wr:
            step_count2_in_[0] = read_buffer_[i+1];
            step_count2_in_[1] = read_buffer_[i+2];
            step_count2_in_[2] = read_buffer_[i+3];
            step_count2_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step3Wr:
            step_count3_in_[0] = read_buffer_[i+1];
            step_count3_in_[1] = read_buffer_[i+2];
            step_count3_in_[2] = read_buffer_[i+3];
            step_count3_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step4Wr:
            step_count4_in_[0] = read_buffer_[i+1];
            step_count4_in_[1] = read_buffer_[i+2];
            step_count4_in_[2] = read_buffer_[i+3];
            step_count4_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Step5Wr:
            step_count5_in_[0] = read_buffer_[i+1];
            step_count5_in_[1] = read_buffer_[i+2];
            step_count5_in_[2] = read_buffer_[i+3];
            step_count5_in_[3] = read_buffer_[i+4];
            i += 5;
            break;
        case Acc0Wr:
            accelerometer0_in_.x[0] = read_buffer_[i+1];
            accelerometer0_in_.x[1] = read_buffer_[i+2];
            accelerometer0_in_.y[0] = read_buffer_[i+3];
            accelerometer0_in_.y[1] = read_buffer_[i+4];
            accelerometer0_in_.z[0] = read_buffer_[i+5];
            accelerometer0_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Acc1Wr:
            accelerometer1_in_.x[0] = read_buffer_[i+1];
            accelerometer1_in_.x[1] = read_buffer_[i+2];
            accelerometer1_in_.y[0] = read_buffer_[i+3];
            accelerometer1_in_.y[1] = read_buffer_[i+4];
            accelerometer1_in_.z[0] = read_buffer_[i+5];
            accelerometer1_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Acc2Wr:
            accelerometer2_in_.x[0] = read_buffer_[i+1];
            accelerometer2_in_.x[1] = read_buffer_[i+2];
            accelerometer2_in_.y[0] = read_buffer_[i+3];
            accelerometer2_in_.y[1] = read_buffer_[i+4];
            accelerometer2_in_.z[0] = read_buffer_[i+5];
            accelerometer2_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Acc3Wr:
            accelerometer3_in_.x[0] = read_buffer_[i+1];
            accelerometer3_in_.x[1] = read_buffer_[i+2];
            accelerometer3_in_.y[0] = read_buffer_[i+3];
            accelerometer3_in_.y[1] = read_buffer_[i+4];
            accelerometer3_in_.z[0] = read_buffer_[i+5];
            accelerometer3_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Acc4Wr:
            accelerometer4_in_.x[0] = read_buffer_[i+1];
            accelerometer4_in_.x[1] = read_buffer_[i+2];
            accelerometer4_in_.y[0] = read_buffer_[i+3];
            accelerometer4_in_.y[1] = read_buffer_[i+4];
            accelerometer4_in_.z[0] = read_buffer_[i+5];
            accelerometer4_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Acc5Wr:
            accelerometer5_in_.x[0] = read_buffer_[i+1];
            accelerometer5_in_.x[1] = read_buffer_[i+2];
            accelerometer5_in_.y[0] = read_buffer_[i+3];
            accelerometer5_in_.y[1] = read_buffer_[i+4];
            accelerometer5_in_.z[0] = read_buffer_[i+5];
            accelerometer5_in_.z[1] = read_buffer_[i+6];
            i += 7;
            break;
        case Raw0Wr:
            translational_velocities_in_.x[0] = read_buffer_[i+1];
            translational_velocities_in_.x[1] = read_buffer_[i+2];
            i += 3;
            break;
        case Raw1Wr:
            translational_velocities_in_.y[0] = read_buffer_[i+1];
            translational_velocities_in_.y[1] = read_buffer_[i+2];
            i += 3;
            break;
        case Raw2Wr:
            translational_velocities_in_.z[0] = read_buffer_[i+1];
            translational_velocities_in_.z[1] = read_buffer_[i+2];
            i += 3;
            break;
        case Raw3Wr:
            actuator_velocities_in_.x[0] = read_buffer_[i+1];
            actuator_velocities_in_.x[1] = read_buffer_[i+2];
            i += 3;
            break;
        case Raw4Wr:
            actuator_velocities_in_.y[0] = read_buffer_[i+1];
            actuator_velocities_in_.y[1] = read_buffer_[i+2];
            i += 3;
            break;
        case Raw5Wr:
            actuator_velocities_in_.z[0] = read_buffer_[i+1];
            actuator_velocities_in_.z[1] = read_buffer_[i+2];
            i += 3;
            break;
        case 0x00:
            if(read_buffer_[i+1]==0x00){if(read_buffer_[i+2]==0x00){
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
            printf("Unknown Code  ");
            printf("%#X\n",read_buffer_[i]);
            i += 1;
            break;
    }} 
}


void SerialPort::ClearReadBuff() {
    for(int i = 0; i < max_packet_size_; ++i){
        read_buffer_[i] = 0x00;
    }
}

void SerialPort::ClearWriteBuff() {
    for(int i = 0; i < max_packet_size_; ++i){
        write_buffer_[i] = 0x00;
    }
    packet_size_ = 0;
}

void SerialPort::ClearRequestBuff() {
    for(int i = 0; i < 1024; ++i){
        request_buffer_[i] = 0x00;
    }
    request_buffer_first_empty_ = 0;
}

void SerialPort::ClearStructAccelBytes(AccelBytes *structure_ptr) {
    (*structure_ptr).x[0] = 0x00;
    (*structure_ptr).x[1] = 0x00;
    (*structure_ptr).y[0] = 0x00;
    (*structure_ptr).y[1] = 0x00;
    (*structure_ptr).z[0] = 0x00;
    (*structure_ptr).z[1] = 0x00;
}

void SerialPort::ClearStructVelBytes(VelBytes *structure_ptr) {
    (*structure_ptr).x[0] = 0x00;
    (*structure_ptr).x[1] = 0x00;
    (*structure_ptr).y[0] = 0x00;
    (*structure_ptr).y[1] = 0x00;
    (*structure_ptr).z[0] = 0x00;
    (*structure_ptr).z[1] = 0x00;
}

//Write a command to a packet
//Creating a packet if one does not already exist

void SerialPort::AddToPacket(unsigned char command) {
    if(packet_size_ == 0) {
        write_buffer_[0] = 0xFF; //We open all packets with a 0xFF
        packet_size_ = 1;
    }
    switch(command) {
            //We write the command and then follow it with any needed data
        case SetRaw0:
            write_buffer_[packet_size_] = SetRaw0;
            write_buffer_[packet_size_+1] = motor_velocities_out_.x[0];
            write_buffer_[packet_size_+2] = motor_velocities_out_.x[1];
            packet_size_ = packet_size_ + 3;
            break;
        case SetRaw1:
            write_buffer_[packet_size_] = SetRaw1;
            write_buffer_[packet_size_+1] = motor_velocities_out_.y[0];
            write_buffer_[packet_size_+2] = motor_velocities_out_.y[1];
            packet_size_ = packet_size_ + 3;
            break;
        case SetRaw2:
            write_buffer_[packet_size_] = SetRaw2;
            write_buffer_[packet_size_+1] = motor_velocities_out_.z[0];
            write_buffer_[packet_size_+2] = motor_velocities_out_.z[1];
            packet_size_ = packet_size_ + 3;
            break;
        case SetRaw3:
            write_buffer_[packet_size_] = SetRaw3;
            write_buffer_[packet_size_+1] = actuator_velocities_out_.x[0];
            write_buffer_[packet_size_+2] = actuator_velocities_out_.x[1];
            packet_size_ = packet_size_ + 3;
            break;
        case SetRaw4:
            write_buffer_[packet_size_] = SetRaw4;
            write_buffer_[packet_size_+1] = actuator_velocities_out_.y[0];
            write_buffer_[packet_size_+2] = actuator_velocities_out_.y[1];
            packet_size_ = packet_size_ + 3;
            break;
        case SetRaw5:
            write_buffer_[packet_size_] = SetRaw5;
            write_buffer_[packet_size_+1] = actuator_velocities_out_.z[0];
            write_buffer_[packet_size_+2] = actuator_velocities_out_.z[1];
            packet_size_ = packet_size_ + 3;
            break;
        default:
            //We write the command into the write buffer followed by two NULL bytes
            write_buffer_[packet_size_] = command;
            write_buffer_[packet_size_+1] = 0x00;
            write_buffer_[packet_size_+2] = 0x00;
            packet_size_ = packet_size_ + 3;
            break;
    }
}


//Write the requested command into the request buffer 
int SerialPort::Request(unsigned char command) {
    if(request_buffer_first_empty_ == 1024) {
        printf("Request buffer full\n");
        return -1;
    }
    else {
        request_buffer_[request_buffer_first_empty_] = command;
        request_buffer_first_empty_ += 1;
        return 0;
    }
}

void SerialPort::PacketManager() {
    int requested_incoming_byte_count = 0;
    int requested_outgoing_byte_count = 0;

    for(int i = 0; i < request_buffer_first_empty_; ++i) {
        switch(request_buffer_[i]) {
          case RUNTIME:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step0Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step1Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step2Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step3Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step4Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Step5Wr:
              requested_incoming_byte_count += 5;
              requested_outgoing_byte_count += 3;
              break;
          case Acc0Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Acc1Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Acc2Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Acc3Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Acc4Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Acc5Wr:
              requested_incoming_byte_count += 7;
              requested_outgoing_byte_count += 3;
              break;
          case Raw0Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          case Raw1Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          case Raw2Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          case Raw3Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          case Raw4Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          case Raw5Wr:
              requested_incoming_byte_count += 3;
              requested_outgoing_byte_count += 3;
              break;
          default:
              requested_outgoing_byte_count += 3;
              break;
        }
        AddToPacket(request_buffer_[i]);

        //If either the incoming or outgoing packets are full we send the messages
        if(requested_outgoing_byte_count > 57 | requested_incoming_byte_count > 53) {
            WriteMessage();
            usleep(360);
            if(requested_incoming_byte_count > 0) {
                ReadMessage();
            }
            requested_outgoing_byte_count = 0;
            requested_incoming_byte_count = 0;
        }
    } 

    //If there is a message left to send we send it
    if(requested_outgoing_byte_count > 0) {
        WriteMessage();
        usleep(360);
        if(requested_incoming_byte_count > 0) {
            ReadMessage();
        }
    }
    ClearRequestBuff();
}

