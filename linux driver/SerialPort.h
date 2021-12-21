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

            VelBytes translational_velocities_out_;//Byte arrays to store the outgoing velocities
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



        private:
            int teensy_;
            unsigned char write_buffer_ [64];
            unsigned char read_buffer_ [64];
            void ClearReadBuff();
            void ClearWriteBuff();
            void ClearStructVelBytes(VelBytes *structure);
            void ClearStructAccelBytes(AccelBytes *structure);
            

    }; 
}

