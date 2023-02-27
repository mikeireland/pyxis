cmake_minimum_required(VERSION 3.18)

project(CoarseMetServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(CoarseMetServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runFLIRCam.cpp ../libs/camera/src/FLIRCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/FLIRcamServerFuncs.cpp CoarseMetrologyServer.cpp
)


target_include_directories(CoarseMetServer PUBLIC ../libs/camera/include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(CoarseMetServer PUBLIC cxx_std_17)

add_subdirectory(../libs/imageproc)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_library(SPINNAKER_LIBRARY Spinnaker HINTS /opt/spinnaker/lib)

target_link_libraries(CoarseMetServer PUBLIC
    commander::commander
    ${SPINNAKER_LIBRARY}
    cfitsio
    image
    pthread
   ${OpenCV_LIBS}
)
