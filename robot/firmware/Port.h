//Object to control the I/O functions of the robot
class Port {
  private:
    static const int packet_size_ = 64;
  
    void EmptyReadBuffer() {
      for(int i = 0; i < packet_size_; ++i){
          read_buffer_[i] = 0x00;
        }
    }

    void EmptyWriteBuffer() {
      for(int i = 0; i < packet_size_; ++i){
          write_buffer_[i] = 0x00;
        }
    }
    
  public:
    byte read_buffer_[packet_size_] = {0x00};
    byte write_buffer_[packet_size_] = {0x00};
    unsigned int first_empty_ = 0;
    unsigned int read_timer_ = 0;
    unsigned int write_timer_ = 0;

    void WriteMessage(){
      unsigned int timer_start = micros();
      for(int j = 0; j < packet_size_; j++){
        Serial.write(write_buffer_[j]);
      }
      Serial.send_now();
      EmptyWriteBuffer();
      first_empty_ = 0;
      unsigned int timer_end = micros();
      write_timer_ = timer_end-timer_start; // Measure the time that writing of the message took in microseconds
    }

    void ReadMessage(){
      EmptyReadBuffer();
      int j = 0;
      unsigned int timer_start = micros();
      while(Serial.available()>0){
        read_buffer_[j] = Serial.read();
        j += 1;
      }
      unsigned int timer_end = micros();
      read_timer_ = timer_end-timer_start; // Measure the time that reading of the message took in microseconds
    }
};
