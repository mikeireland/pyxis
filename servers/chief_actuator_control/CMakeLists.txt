cmake_minimum_required(VERSION 3.18)

project(ChiefAuxServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(ChiefAuxServer
    src/main.cpp src/chiefAuxGlobals.cpp src/chiefAuxServerFuncs.cpp
)

target_include_directories(ChiefAuxServer PUBLIC include ../libs/camera/include)
target_compile_features(ChiefAuxServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)

target_link_libraries(ChiefAuxServer PUBLIC
    commander::commander
    pthread
)
