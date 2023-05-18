//SerialPort.cpp
#include "SerialPort.h"
#include <iostream>
#include <iomanip>

using namespace Comms;

int SerialPort::max_packet_size_ = 64; 

//Class constructor to sets up the serial connection to the teensy correctly
//We also zero all of the input registers
SerialPort::SerialPort(int device_id_target) {
    //Open comms with the first teensy we can find
    init = true;
    sleep(30);
    OpenPort();

    //ID the opened teensy
    ReadMessage();
    Request(TID);
	SendAllRequests();
	sleep(20);
	ReadMessage();
	printf("Device ID: %u\n",device_id_);
	printf("Device Firmware Version: %u\n",device_firmware_v_);

    //If it is wrong we close it and search the rest of the device files
	while((device_id_ != device_id_target) && (device_file_index_ < 16)){
		//ID the teensy
		ClosePort();
		teensy_ = -1;
		OpenPort();
		Request(TID);
		PacketManager();
		printf("Device ID: %u\n",device_id_);
		printf("Device Firmware Version: %u\n",device_firmware_v_);
	}

	//We allow two attempts for convenience
	device_file_index_ = 0;

	while((device_id_ != device_id_target) && (device_file_index_ < 16)){
		//ID the teensy
		ClosePort();
		teensy_ = -1;
		OpenPort();
		Request(TID);
		PacketManager();
		printf("Device ID: %u\n",device_id_);
		printf("Device Firmware Version: %u\n",device_firmware_v_);
	}
	init = false;
	//serial_read_thread = std::thread(&SerialPort::ReadMessageAsync, this);
    //sch_params.sched_priority = 97;
    //pthread_setschedparam(serial_read_thread.native_handle(), SCHED_RR, &sch_params);
}

void SerialPort::OpenPort() {
    char device_file_buffer [64] = {""};
    while((teensy_ < 0) && (device_file_index_ < 16)) {

        sprintf(device_file_buffer,"/dev/ttyACM%d",device_file_index_);
        teensy_ = open(device_file_buffer,O_RDWR);
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

        device_file_index_ = device_file_index_ + 1;
    }
    if(device_file_index_ == 16) {
        printf("WARNING: teensy could not be found\n");
    }
}

//Closes the communication with the teensy
void SerialPort::ClosePort() {
    init = true;
    serial_read_thread.join();
    sleep(1);
    close(teensy_);
    printf("Comms Closed\n");
}

void SerialPort::WriteMessage() {
    std::cout << "send:";
            for(int i = 0; i < sizeof(write_buffer_); ++i) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)write_buffer_[i] << std::dec << ':';
            }
            std::cout << "\n";
    memcpy(last_pack_, write_buffer_, sizeof(write_buffer_));
    write(teensy_,write_buffer_,sizeof(write_buffer_));
    ClearWriteBuff();
}

void SerialPort::ReadMessageAsync() {
    while (!init) {
        ReadMessage();
        usleep(40);
    }
}

void SerialPort::ReadMessage() {
  
        ClearReadBuff();
        read(teensy_,&read_buffer_,sizeof(read_buffer_));
        
        if (read_buffer_[0]) {
            std::cout << "receive:";
            for(int i = 0; i < sizeof(read_buffer_); ++i) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)read_buffer_[i] << std::dec << ':';
            }
            std::cout << "\n";
        }
        int i = 0;
        while(i < sizeof(read_buffer_)){
        switch(read_buffer_[i]){
            case LEDON:
                ledOn = true;
                i += 3;
                break;
            case LEDOFF:
                ledOn = false;
                i += 3;
                break;
            case TID:
                device_id_ = read_buffer_[i+1];
                device_firmware_v_ = read_buffer_[i+2];
                i += 3;
                break;
            case 0x00:
                //if(read_buffer_[i+1]==0x00){if(read_buffer_[i+2]==0x00){
                //    i = 64;//If we have reached three consecutive nulls, end the loop
                //    break;
                //    }}
                i += 1;
                break;
            case UnrecVal:
                printf("Unrecognised Command\n");
                i += 1;
                break;
            case PackFail:
                printf("Packet Start Not Present\n");
                /*
                std::cout << "sent:";
                for(int i = 0; i < sizeof(last_pack_); ++i) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)last_pack_[i] << std::dec << ':';
                }
                std::cout << "\n";
                std::cout << "receive:";
                for(int i = 0; i < sizeof(read_buffer_); ++i) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)read_buffer_[i] << std::dec << ':';
                }
                std::cout << "\n";
                */
                i = 64;
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
    if(request_buffer_first_empty_ >= 1024) {
        std::cout << "Request buffer full\n";
        return -1;
    }
    else {
        request_buffer_[request_buffer_first_empty_] = command;
        request_buffer_first_empty_ += 1;
        return 0;
    }
}

void SerialPort::SendAllRequests() {
    for(int i = 0; i < request_buffer_first_empty_; ++i) {
        AddToPacket(request_buffer_[i]);
    }
    WriteMessage();
    ClearRequestBuff();
}

void SerialPort::PacketManager() {
    int requested_incoming_byte_count = 0;
    int requested_outgoing_byte_count = 0;

    for(int i = 0; i < request_buffer_first_empty_; ++i) {
        switch(request_buffer_[i]) {
          case TID:
               requested_incoming_byte_count += 3;
               requested_outgoing_byte_count += 3;
          default:
              requested_outgoing_byte_count += 3;
              break;
        }
        AddToPacket(request_buffer_[i]);

        //If either the incoming or outgoing packets are full we send the messages
        if(requested_outgoing_byte_count > 57 | requested_incoming_byte_count > 53) {
            /*
            std::cout << "request:";
            for(int i = 0; i < request_buffer_first_empty_; ++i) {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)request_buffer_[i] << std::dec << ':';
            }*/
            WriteMessage();
            usleep(10);
            if(requested_incoming_byte_count > 0 && init) {
                usleep(300);
                ReadMessage();
            }
            requested_outgoing_byte_count = 0;
            requested_incoming_byte_count = 0;
        }
    } 

    //If there is a message left to send we send it
    if(requested_outgoing_byte_count > 0) {
        /*
        std::cout << "request:";
        for(int i = 0; i < request_buffer_first_empty_; ++i) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)request_buffer_[i] << std::dec << ':';
        }*/
        WriteMessage();
        usleep(10);
        if(requested_incoming_byte_count > 0 && init) {
            usleep(300);
            ReadMessage();
        }
    }
    //if (max_req_buf < request_buffer_first_empty_) {
    //    max_req_buf = request_buffer_first_empty_;
    //}
    //std::cout << max_req_buf << ';';
    //for(int i = 0; i < request_buffer_first_empty_; ++i) {
    //    std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)request_buffer_[i] << std::dec << ':';
    //}
    ClearRequestBuff();
}

