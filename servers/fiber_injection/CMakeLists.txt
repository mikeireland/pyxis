cmake_minimum_required(VERSION 3.18)

project(FiberInjectionServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(FiberInjectionServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runFLIRCam.cpp ../libs/camera/src/FLIRCamera.cpp ../libs/camera/src/globals.cpp ../libs/imageproc/src/centroid.cpp ../libs/camera/src/FLIRcamServerFuncs.cpp FiberInjectionServer.cpp
)

target_include_directories(FiberInjectionServer PUBLIC ../libs/camera/include ../libs/imageproc/include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(FiberInjectionServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_library(SPINNAKER_LIBRARY Spinnaker HINTS /opt/spinnaker/lib)

target_link_libraries(FiberInjectionServer PUBLIC
    commander::commander
    ${SPINNAKER_LIBRARY}
    cfitsio
    pthread
   ${OpenCV_LIBS}
)
