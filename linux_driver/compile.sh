g++ -c Decode.cpp -lgsl -lgslcblas -lm
g++ -c SerialPort.cpp -lgsl -lgslcblas -lm
g++ -c Servo.cpp -lgsl -lgslcblas -lm
g++ -c RobotDriver.cpp -lgsl -lgslcblas -lm
g++ -c RobotDriverMain.cpp -lgsl -lgslcblas -lm
g++ RobotDriverMain.o RobotDriver.o SerialPort.o Servo.o Decode.o -static -lgsl -lgslcblas -lm -o run.out

g++ -c AccelerationTestSource.cpp -lgsl -lgslcblas -lm
g++ AccelerationTestSource.o RobotDriver.o SerialPort.o Servo.o Decode.o -static -lgsl -lgslcblas -lm -o AccTest.out