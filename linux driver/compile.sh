g++ -c SerialPort.cpp -lgsl -lgslcblas -lm
g++ -c Servo.cpp -lgsl -lgslcblas -lm
g++ -c RobotDriver.cpp -lgsl -lgslcblas -lm
g++ RobotDriver.o Servo.o SerialPort.o -static -lgsl -lgslcblas -lm -o run.out
