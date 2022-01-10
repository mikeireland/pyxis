//SerialPort.h
#pragma once

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
            static int max_packet_size_;
            int packet_size_;
            SerialPort();
            void ClosePort();
            void ReadMessage();
            void WriteMessage();
            void AddToPacket(unsigned char command);
            int Request(unsigned char command);

            void PacketManager();
            

            VelBytes motor_velocities_out_;//Byte arrays to store the outgoing velocities
            VelBytes actuator_velocities_out_; //For these take x,y,z == 0,1,2

            VelBytes translational_velocities_in_;//Byte arrays to store the incoming velocities
            VelBytes actuator_velocities_in_; //For these take x,y,z == 0,1,2

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


        private:
            int teensy_;
            unsigned char write_buffer_ [64];
            unsigned char read_buffer_ [64];
            unsigned char request_buffer_ [1024];
            int request_buffer_first_empty_ = 0;


            void ClearReadBuff();
            void ClearWriteBuff();
            void ClearRequestBuff();
            void ClearStructVelBytes(VelBytes *structure);
            void ClearStructAccelBytes(AccelBytes *structure);
    }; 
}

