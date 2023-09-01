cmake_minimum_required(VERSION 3.18)

project(FineStarTrackerServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(FineStarTrackerServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runFLIRCam.cpp ../libs/camera/src/FLIRCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/FLIRcamServerFuncs.cpp ../libs/imageproc/src/centroid.cpp FineStarTrackerServer.cpp
)

target_include_directories(FineStarTrackerServer PUBLIC ../libs/camera/include ../libs/imageproc/include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(FineStarTrackerServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_library(SPINNAKER_LIBRARY Spinnaker HINTS /opt/spinnaker/lib)

target_link_libraries(FineStarTrackerServer PUBLIC
    commander::commander
    ${SPINNAKER_LIBRARY}
    cfitsio
    pthread
   ${OpenCV_LIBS}
)
