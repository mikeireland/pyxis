cmake_minimum_required(VERSION 3.18)

project(ChiefAccServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(ChiefAccServer
    src/main.cpp src/ChiefAccGlobals.cpp src/ChiefAccServerFuncs.cpp
)

target_include_directories(ChiefAccServer PUBLIC include)
target_compile_features(ChiefAccServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)

target_link_libraries(ChiefAccServer PUBLIC
    commander::commander
    pthread
)
