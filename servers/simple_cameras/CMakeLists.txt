cmake_minimum_required(VERSION 3.18)

project(SimpleFLIRServer VERSION 0.1.0 LANGUAGES CXX)

add_executable(SimpleFLIRServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runFLIRCam.cpp ../libs/camera/src/FLIRCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/FLIRcamServerFuncs.cpp SimpleFLIRCam.cpp
)

target_include_directories(FLIRServer PUBLIC ../libs/camera/include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(FLIRServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_library(SPINNAKER_LIBRARY Spinnaker HINTS /opt/spinnaker/lib)

target_link_libraries(FLIRServer PUBLIC
    commander::commander
    ${SPINNAKER_LIBRARY}
    cfitsio
    pthread
   ${OpenCV_LIBS}
)