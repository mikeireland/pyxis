//SerialPort.h
#ifndef SERIAL_PORT_H_INCLUDE_GUARD
#define SERIAL_PORT_H_INCLUDE_GUARD

// C library headers
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <thread>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/file.h> // Contains the flock command

//Macro headers
#include "Commands.h"
#include "ErrorCodes.h"

#include "Decode.h"

namespace Comms
{   
    //Structures to store the acceleration and velocity values
    struct AccelBytes 
    {
        unsigned char x [2], y[2], z [2];
    };
    struct VelBytes
    {
        unsigned char x [2], y[2], z [2];
    };

    class SerialPort
    {
        public:
            SerialPort(int device_id_target);
            
            static int max_packet_size_;
            int packet_size_;
            void OpenPort();
            void ClosePort();
            void ReadMessageAsync();
            int ReadMessage();
            void WriteMessage();
            void SendAllRequests();
            void AddToPacket(unsigned char command);
            int Request(unsigned char command);
            bool ledOn = false;

            int teensy_ = -1;
            void PacketManager();
            

            VelBytes motor_velocities_out_;//Byte arrays to store the outgoing velocities
            VelBytes actuator_velocities_out_; //For these take x,y,z == 0,1,2

            VelBytes translational_velocities_in_;//Byte arrays to store the incoming velocities
            VelBytes actuator_velocities_in_; //For these take x,y,z == 0,1,2
            
            unsigned char goniometer_velocity_out_ [2];
            
            int16_t PC_Voltage;
            int16_t PC_Current;
            int16_t Motor_Voltage;
            int16_t Motor_Current;

            AccelBytes accelerometer0_in_;//Byte arrays to store the incoming accelerations
            AccelBytes accelerometer1_in_;
            AccelBytes accelerometer2_in_;
            AccelBytes accelerometer3_in_;
            AccelBytes accelerometer4_in_;
            AccelBytes accelerometer5_in_;

            unsigned char runtime_in_ [4];//Byte array to store the incoming runtime
            unsigned char step_count0_in_[4] = {0x00}; //Byte arrays to store in incoming step_counts
            unsigned char step_count1_in_[4] = {0x00};
            unsigned char step_count2_in_[4] = {0x00};
            unsigned char step_count3_in_[4] = {0x00};//Actuators 0,1,2 and indexes 3,4,5
            unsigned char step_count4_in_[4] = {0x00};
            unsigned char step_count5_in_[4] = {0x00};

            unsigned char device_id_; //bytes to store the current device being commed with
            unsigned char device_firmware_v_;
            int device_file_index_ = 0;



        private:
            unsigned char write_buffer_ [128];
            unsigned char read_buffer_ [128];
            unsigned char last_pack_ [128];
            unsigned char request_buffer_ [1024];
            int request_buffer_first_empty_ = 0;
            int max_req_buf = 0;
            bool init = false;
            sched_param sch_params;
            //std::thread serial_read_thread;

            void ClearReadBuff();
            void ClearWriteBuff();
            void ClearRequestBuff();
            void ClearStructVelBytes(VelBytes *structure);
            void ClearStructAccelBytes(AccelBytes *structure);
    }; 
}

#endif
