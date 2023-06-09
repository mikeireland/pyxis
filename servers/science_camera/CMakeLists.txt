cmake_minimum_required(VERSION 3.18)

project(SciCamServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(SciCamServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runQHYCam.cpp ../libs/camera/src/QHYCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/QHYcamServerFuncs.cpp ../libs/brent/brent.cpp src/SciCamServer.cpp src/setup.cpp src/group_delay.cpp
)

set_source_files_properties(
  src/group_delay.cpp
  PROPERTIES
    COMPILE_OPTIONS "-O2"
)

target_include_directories(SciCamServer PUBLIC ../libs/camera/include ../libs/brent include ../libs)
target_compile_features(SciCamServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

target_link_libraries(SciCamServer PUBLIC
    commander::commander
    qhyccd
    cfitsio
    pthread
    fftw3
   ${OpenCV_LIBS}
)
