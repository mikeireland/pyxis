cmake_minimum_required(VERSION 3.18)

project(StateMachineServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(StateMachineServer
    main.cpp
)

target_compile_features(StateMachineServer PUBLIC cxx_std_17)
target_include_directories(StateMachineServer PUBLIC ../libs/camera/include)

find_package(commander REQUIRED)

target_link_libraries(StateMachineServer PUBLIC
    commander::commander
)
