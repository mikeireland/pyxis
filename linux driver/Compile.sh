g++ -c SerialPort.cpp
g++ -c Servo.cpp
g++ -c RobotDriver.cpp
g++ RobotDriver.o Servo.o SerialPort.o -o run.out