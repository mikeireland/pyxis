cmake_minimum_required(VERSION 3.18)

project(DeputyAuxServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(DeputyAuxServer
    src/main.cpp src/deputyAuxGlobals.cpp src/deputyAuxServerFuncs.cpp src/Decode.cpp src/SerialPort.cpp
)

target_include_directories(DeputyAuxServer PUBLIC include ../libs/camera/include)
target_compile_features(DeputyAuxServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)

target_link_libraries(DeputyAuxServer PUBLIC
    commander::commander
    pthread
)
