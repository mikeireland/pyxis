cmake_minimum_required(VERSION 3.18)

project(robot_driver VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(robot_driver
    src/robot_server_main.cpp src/robot_server.cpp src/RobotDriver.cpp src/Decode.cpp src/SerialPort.cpp src/Servo.cpp
)

target_include_directories(robot_driver PUBLIC include)
target_compile_features(robot_driver PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package(GSL REQUIRED)

target_link_libraries(robot_driver PUBLIC
    commander::commander
    GSL::gsl
    GSL::gslcblas
)
