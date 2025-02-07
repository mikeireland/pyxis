#include <SPI.h>

//These header files must be conained in the same directory as the firmware
#include "Decode.h"
#include "MotorDriver.h"
#include "AccelerometerReader.h"
#include "Port.h"
#include "Commands.h"
#include "ErrorCodes.h"

//This integer notes that the device is the motion control teensy and the second value 
//is the firmware version for motion control
unsigned short int DeviceID  = 128;
unsigned short int FirmwareV = 2;

unsigned int scheduler_time_longest = 0;

class Controller {
  private:
    static const int sched_length_ = 50;
    int sched_state_ = 0;
    int sched_[sched_length_] = {0x00}; //Since variable length arrays are not allowed in C++ we just define an array which is too big
  
    
    short int v_int_ [7] = {0}; 
    
    AccelerometerReader accelerometer_reader;
    struct Triple accel_buffer_[6];

    Port port;
    unsigned int temp_ = 0; //This will be used to temporarily store the next available memory write buffer location

    void SetDefaultSchedule(){
    //Sets the initial controller schedule to read the accelerometers every 1ms and to write the values to serial.
      sched_[0] = Acc0ReX;
      sched_[1] = Acc0ReY;
      sched_[2] = Acc0ReZ;
      sched_[3] = Acc1ReX;
      sched_[4] = Acc1ReY;
      sched_[5] = Acc1ReZ;
      sched_[6] = Acc2ReX;
      sched_[7] = Acc2ReY;
      sched_[8] = Acc2ReZ;
      sched_[9] = Acc3ReX;
      sched_[10] = Acc3ReY;
      sched_[11] = Acc3ReZ;
      sched_[12] = Acc4ReX;
      sched_[13] = Acc4ReY;
      sched_[14] = Acc4ReZ;
      sched_[15] = Acc5ReX;
      sched_[16] = Acc5ReY;
      sched_[17] = Acc5ReZ;
    }

    void UpdatePositions() {
      motor_driver.UpdatePositions();
    }

    void SetRawVelocity(int motor_index, float v) {
      motor_driver.SetRawVelocity(motor_index, v);
    }

    //Reads the incoming packet and sets the schedule in accordance with the commands in the packet
    void DecodePacket(){
      if(port.read_buffer_[0] == 0xFF){
      //We loop over the message portion of the packet
        for(int i = 1; i < 125; i = i+3){
        switch(port.read_buffer_[i]){
          case SetRaw0:
            //Read the input velocity and write it into the motor driver
            v_int_[0] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(0,v_int_[0]);
            break;
          case SetRaw1:
            //Read the input velocity and write it into the motor driver
            v_int_[1] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(1,v_int_[1]);
            break;
          case SetRaw2:
            //Read the input velocity and write it into the motor driver
            v_int_[2] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(2,v_int_[2]);
            break;
          case SetRaw3:
            //Read the input velocity and write it into the motor driver
            v_int_[3] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(3,v_int_[3]);
            break;
          case SetRaw4:
            //Read the input velocity and write it into the motor driver
            v_int_[4] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(4,v_int_[4]);
            break;
          case SetRaw5:
            //Read the input velocity and write it into the motor driver
            v_int_[5] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(5,v_int_[5]);
            break;
          case SetRaw6:
            //Read the input velocity and write it into the motor driver
            v_int_[6] = (port.read_buffer_[i+1] << 8) | (port.read_buffer_[i+2] << 0);
            motor_driver.SetRawVelocity(6,v_int_[6]);
            break;
          case Acc0Wr:
            if(ScheduleToNextFree(Acc0Wr) == 0){ //We check if the task was scheduled and if it was then we move on
              break;
            } else {
              ReportError(SchedFull); //We send back an error code if the task couldn't be scheduled
              break;
            }
          case Acc1Wr:
            if(ScheduleToNextFree(Acc1Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Acc2Wr:
            if(ScheduleToNextFree(Acc2Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Acc3Wr:
            if(ScheduleToNextFree(Acc3Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Acc4Wr:
            if(ScheduleToNextFree(Acc4Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Acc5Wr:
            if(ScheduleToNextFree(Acc5Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          
          case Raw0Wr:
            if(ScheduleToNextFree(Raw0Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Raw1Wr:
            if(ScheduleToNextFree(Raw1Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Raw2Wr:
            if(ScheduleToNextFree(Raw2Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Raw3Wr:
            if(ScheduleToNextFree(Raw3Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Raw4Wr:
            if(ScheduleToNextFree(Raw4Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Raw5Wr:
            if(ScheduleToNextFree(Raw5Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step0Wr:
            if(ScheduleToNextFree(Step0Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step1Wr:
            if(ScheduleToNextFree(Step1Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step2Wr:
            if(ScheduleToNextFree(Step2Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step3Wr:
            if(ScheduleToNextFree(Step3Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step4Wr:
            if(ScheduleToNextFree(Step4Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case Step5Wr:
            if(ScheduleToNextFree(Step5Wr) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          
          case RUNTIME:
            if(ScheduleToNextFree(RUNTIME) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          case STOP: //When we receive a STOP command we set all motor velocities to zero
            motor_driver.SetRawVelocity(0,0);
            motor_driver.SetRawVelocity(1,0);
            motor_driver.SetRawVelocity(2,0);
            motor_driver.SetRawVelocity(3,0);
            motor_driver.SetRawVelocity(4,0);
            motor_driver.SetRawVelocity(5,0);
            motor_driver.SetRawVelocity(6,0);
            break;

          case ResetSteps: //When we receive a ResetStepCount command we set all step count varibles to zero
            motor_driver.ResetStepCount();
            break;

          case ID: //We we receive an ID request we send back the DeviceID
            if(ScheduleToNextFree(ID) == 0){ 
              break;
            } else {
              ReportError(SchedFull);
              break;
            }
          
          case EMPTY:
            break;
            
          default:
            //If we receive a value we don't recognise, we report an error to the host
            ReportError(UnrecVal);
            break;
          }
        }
      }
      else{
        //If the packet doesn't begin correctly we report an error to the host
        ReportError(PackFail);
      }
    }

    int ScheduleToNextFree(byte task){
      for(int j = sched_state_+1; j < sched_length_; ++j){
        if(sched_[j] == EMPTY){
          sched_[j] = task;
          return 0;
          }
      }
      for(int j = 0; j < sched_state_; ++j){
        if(sched_[j] == EMPTY){
          sched_[j] = task;
          return 0;
        }
       }
       return -1;//In the event that it fails to find a spot in the schedule we return a -1
    }

    void WriteAcc(int index){
      //Decode and push the values from an accelerometer into the Port's write_buffer

      //Check if the write buffer is full, and if so send the message (recall that we want our packets to end in three 0x00s hence we 
       //only fill up to index 61)
      temp_ = port.first_empty_;
      if(port.first_empty_ + 7 > 125){
        port.WriteMessage();
      }
      
      temp_ = port.first_empty_;
      switch(index){
        case 0:
          port.write_buffer_[temp_] = Acc0Wr;
          break;
        case 1:
          port.write_buffer_[temp_] = Acc1Wr;
          break;
        case 2:
          port.write_buffer_[temp_] = Acc2Wr;
          break;
        case 3:
          port.write_buffer_[temp_] = Acc3Wr;
          break;
        case 4:
          port.write_buffer_[temp_] = Acc4Wr;
          break;
        case 5:
          port.write_buffer_[temp_] = Acc5Wr;
          break;
      }
      port.write_buffer_[temp_+1] = accel_buffer_[index].x[0];
      port.write_buffer_[temp_+2] = accel_buffer_[index].x[1];
      port.write_buffer_[temp_+3] = accel_buffer_[index].y[0];
      port.write_buffer_[temp_+4] = accel_buffer_[index].y[1];
      port.write_buffer_[temp_+5] = accel_buffer_[index].z[0];
      port.write_buffer_[temp_+6] = accel_buffer_[index].z[1];
      port.first_empty_ = temp_ + 7;
    }

    void WriteVel(int index){
      //write one of the velocities of the robot into the Port's write_buffer
      //For convenience we send it as an int which represents the velocity in microm/s

       //Check if the write buffer is full, and if so send the message (recall that we want our packets to end in three 0x00s hence we 
       //only fill up to index 61)
      temp_ = port.first_empty_;
      if(temp_ + 3 > 125){
        port.WriteMessage();
      }

      //Write into the write buffer
      temp_ = port.first_empty_;
      switch(index){
        case 0:
          port.write_buffer_[temp_] = Raw0Wr;
          break;
        case 1:
          port.write_buffer_[temp_] = Raw1Wr;
          break;
        case 2:
          port.write_buffer_[temp_] = Raw2Wr;
          break;
        case 3:
          port.write_buffer_[temp_] = Raw3Wr;
          break;
        case 4:
          port.write_buffer_[temp_] = Raw4Wr;
          break;
        case 5:
          port.write_buffer_[temp_] = Raw5Wr;
          break;
      }
      short int v_temp = 1000000*motor_driver.motor_vels_[index];
      ShortIntToBytes(v_temp, &port.write_buffer_[temp_+1], &port.write_buffer_[temp_+2]);
      port.first_empty_ = temp_ + 3;
    }

        //Write the runtime value into the message buffer.
    void WriteRuntime(){;
        //Check if the write buffer is full, and if so send the message (recall that we want our packets to end in three 0x00s hence we 
        //only fill up to index 61)
        temp_ = port.first_empty_;
        if(temp_ + 5 > 125){
          port.WriteMessage();
        }
      
        temp_ = port.first_empty_;
        port.write_buffer_[temp_] = RUNTIME; //We repeat the command call as an acknowledgement and to indicate the next 4 bytes are the value
        IntToBytes(scheduler_time_longest, &port.write_buffer_[temp_+1], &port.write_buffer_[temp_+2], &port.write_buffer_[temp_+3], &port.write_buffer_[temp_+4]);
        port.first_empty_ = temp_+5; //update the first empty value in the write buffer
    }

    //Write a steps value into the message buffer.
    void WriteSteps(int index){;
        //Check if the write buffer is full, and if so send the message (recall that we want our packets to end in three 0x00s hence we 
        //only fill up to index 61)
        temp_ = port.first_empty_;
        if(temp_ + 5 > 125){
          port.WriteMessage();
        }
      
        temp_ = port.first_empty_;
        //We repeat the command call as an acknowledgement and to indicate the next 4 bytes are the value
        switch(index){
        case 0:
          port.write_buffer_[temp_] = Step0Wr;
          break;
        case 1:
          port.write_buffer_[temp_] = Step1Wr;
          break;
        case 2:
          port.write_buffer_[temp_] = Step2Wr;
          break;
        case 3:
          port.write_buffer_[temp_] = Step3Wr;
          break;
        case 4:
          port.write_buffer_[temp_] = Step4Wr;
          break;
        case 5:
          port.write_buffer_[temp_] = Step5Wr;
          break;
      }
      
        IntToBytes(motor_driver.step_count_[index], &port.write_buffer_[temp_+1], &port.write_buffer_[temp_+2], &port.write_buffer_[temp_+3], &port.write_buffer_[temp_+4]);
        port.first_empty_ = temp_+5; //update the first empty value in the write buffer
    }

    //Function to grab the Teensy's firmware device ID and push it in a packet
    void WriteID() {
      temp_ = port.first_empty_;
      port.write_buffer_[temp_] = ID;
      port.write_buffer_[temp_+1] = DeviceID;
      port.write_buffer_[temp_+2] = FirmwareV;
      port.first_empty_ = temp_+3;
    }

    //Subroutine to write an error message back to the host.
    void ReportError(byte error_code){
      temp_ = port.first_empty_;
      port.write_buffer_[temp_] = error_code;
      port.first_empty_ = temp_+1;
    }
    
  public:
    MotorDriver motor_driver;
    Controller() {
      SetDefaultSchedule();
    }

    
  
    //This is the main operation. 
    //Each call of the scheduler will pulse the motors and perform one of a sequence of other actions which it reads in sequence from the schedule
    //It will then update the loop state variable so that it doesn't repeat itself until the loop_state has reached its
    //maximum value which we call the loop_length.
    void Scheduler() {
     //Check the limit switches to see if they have been pressed
     motor_driver.CheckLimitSwitches();
      
     //Pulse the motors
     UpdatePositions();

     //the following checks which command code is referenced at this point in the schedule and runs that command
     switch(sched_[sched_state_]){

      //Read the 2 bytes of an accelerometer axis and store it in the acceleration buffer
      case Acc0ReX:
        accelerometer_reader.GetX(0,accel_buffer_[0].x);
        break;
      case Acc0ReY:
        accelerometer_reader.GetY(0,accel_buffer_[0].y);
        break;
      case Acc0ReZ:
        accelerometer_reader.GetZ(0,accel_buffer_[0].z);
        break;
      case Acc1ReX:
        accelerometer_reader.GetX(1,accel_buffer_[1].x);
        break;
      case Acc1ReY:
        accelerometer_reader.GetY(1,accel_buffer_[1].y);
        break;
      case Acc1ReZ:
        accelerometer_reader.GetZ(1,accel_buffer_[1].z);
        break;
      case Acc2ReX:
        accelerometer_reader.GetX(2,accel_buffer_[2].x);
        break;
      case Acc2ReY:
        accelerometer_reader.GetY(2,accel_buffer_[2].y);
        break;
      case Acc2ReZ:
        accelerometer_reader.GetZ(2,accel_buffer_[2].z);
        break;
      case Acc3ReX:
        accelerometer_reader.GetX(3,accel_buffer_[3].x);
        break;
      case Acc3ReY:
        accelerometer_reader.GetY(3,accel_buffer_[3].y);
        break;
      case Acc3ReZ:
        accelerometer_reader.GetZ(3,accel_buffer_[3].z);
        break;
      case Acc4ReX:
        accelerometer_reader.GetX(4,accel_buffer_[4].x);
        break;
      case Acc4ReY:
        accelerometer_reader.GetY(4,accel_buffer_[4].y);
        break;
      case Acc4ReZ:
        accelerometer_reader.GetZ(4,accel_buffer_[4].z);
        break;
      case Acc5ReX:
        accelerometer_reader.GetX(5,accel_buffer_[5].x);
        break;
      case Acc5ReY:
        accelerometer_reader.GetY(5,accel_buffer_[5].y);
        break;
      case Acc5ReZ:
        accelerometer_reader.GetZ(5,accel_buffer_[5].z);
        break;

      //Push the values from the acceleration buffer to the write buffer
      case Acc0Wr:
        WriteAcc(0);
        sched_[sched_state_] = EMPTY;
        break;
      case Acc1Wr:
        WriteAcc(1);
        sched_[sched_state_] = EMPTY;
        break;
      case Acc2Wr:
        WriteAcc(2);
        sched_[sched_state_] = EMPTY;
        break;
      case Acc3Wr:
        WriteAcc(3);
        sched_[sched_state_] = EMPTY;
        break;
      case Acc4Wr:
        WriteAcc(4);
        sched_[sched_state_] = EMPTY;
        break;
      case Acc5Wr:
        WriteAcc(5);
        sched_[sched_state_] = EMPTY;
        break;

      case Raw0Wr:
        WriteVel(0);
        sched_[sched_state_] = EMPTY;
        break;
      case Raw1Wr:
        WriteVel(1);
        sched_[sched_state_] = EMPTY;
        break;
      case Raw2Wr:
        WriteVel(2);
        sched_[sched_state_] = EMPTY;
        break;
      case Raw3Wr:
        WriteVel(3);
        sched_[sched_state_] = EMPTY;
        break;
      case Raw4Wr:
        WriteVel(4);
        sched_[sched_state_] = EMPTY;
        break;
      case Raw5Wr:
        WriteVel(5);
        sched_[sched_state_] = EMPTY;
        break;
      case Step0Wr:
        WriteSteps(0);
        sched_[sched_state_] = EMPTY;
        break;
      case Step1Wr:
        WriteSteps(1);
        sched_[sched_state_] = EMPTY;
        break;
      case Step2Wr:
        WriteSteps(2);
        sched_[sched_state_] = EMPTY;
        break;
      case Step3Wr:
        WriteSteps(3);
        sched_[sched_state_] = EMPTY;
        break;
      case Step4Wr:
        WriteSteps(4);
        sched_[sched_state_] = EMPTY;
        break;
      case Step5Wr:
        WriteSteps(5);
        sched_[sched_state_] = EMPTY;
        break;
      case RUNTIME:
        WriteRuntime();
        sched_[sched_state_] = EMPTY;
        break;
      case ID:
        WriteID();
        sched_[sched_state_] = EMPTY;
        break;
        
      default:
        //If there is a message we read it
        if(Serial.available() > 0){
          port.ReadMessage();
          DecodePacket();
          break;
        } else if(port.write_buffer_[0] != 0x00){//If there is a message to send we send it
          port.WriteMessage();
          break;
        }
          delayMicroseconds(10); //If there is nothing to do we wait slightly (this avoids a memory leak) and break
          break;
      }

      //Move to the next position in the schedule
      sched_state_ += 1;
      if(sched_state_ == sched_length_){
        sched_state_ = 0;
      }
    }
};

// Global instances
IntervalTimer timer;
Controller controller;

void SchedulerWrapper() {
  unsigned int timer_start = micros();
  controller.Scheduler();
  unsigned int timer_end = micros();
  if(timer_end-timer_start > scheduler_time_longest){
    scheduler_time_longest = timer_end-timer_start;
  } 
}

void setup() {
  //controller.motor_driver.SetRawVelocity(3,200);
  //controller.motor_driver.SetRawVelocity(6,-10000);

  timer.begin(SchedulerWrapper, 20);
}


void loop() {
}
