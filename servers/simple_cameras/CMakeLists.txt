cmake_minimum_required(VERSION 3.18)

project(SimpleServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(SimpleFLIRServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runFLIRCam.cpp ../libs/camera/src/FLIRCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/FLIRcamServerFuncs.cpp SimpleFLIRCam.cpp
)

add_executable(SimpleQHYServer
    ../libs/camera/src/main.cpp ../libs/camera/src/runQHYCam.cpp ../libs/camera/src/QHYCamera.cpp ../libs/camera/src/globals.cpp ../libs/camera/src/QHYcamServerFuncs.cpp SimpleQHYCam.cpp
)

target_include_directories(SimpleFLIRServer PUBLIC ../libs/camera/include /opt/spinnaker/include ~/Downloads/build/include)
target_compile_features(SimpleFLIRServer PUBLIC cxx_std_17)

target_include_directories(SimpleQHYServer PUBLIC ../libs/camera/include ~/Downloads/build/include)
target_compile_features(SimpleQHYServer PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_library(SPINNAKER_LIBRARY Spinnaker HINTS /opt/spinnaker/lib)

target_link_libraries(SimpleFLIRServer PUBLIC
    commander::commander
    ${SPINNAKER_LIBRARY}
    cfitsio
    pthread
   ${OpenCV_LIBS}
)

target_link_libraries(SimpleQHYServer PUBLIC
    commander::commander
    qhyccd
    cfitsio
    pthread
   ${OpenCV_LIBS}
)
