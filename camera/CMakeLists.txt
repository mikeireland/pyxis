cmake_minimum_required(VERSION 3.18)

project(FLIRServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(FLIRServer
    src/main.cpp src/runFLIRCam.cpp src/FLIRCamera.cpp src/globals.cpp src/FLIRcamServerFuncs.cpp
)
add_executable(QHYServer
    src/main.cpp src/runQHYCam.cpp src/QHYCamera.cpp src/globals.cpp src/QHYcamServerFuncs.cpp
)

target_include_directories(FLIRServer PUBLIC include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(FLIRServer PUBLIC cxx_std_17)

target_include_directories(QHYServer PUBLIC include ~/Downloads/build/include)
target_compile_features(QHYServer PUBLIC cxx_std_17)

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

target_link_libraries(QHYServer PUBLIC
    commander::commander
    qhyccd
    cfitsio
    pthread
   ${OpenCV_LIBS}
)
