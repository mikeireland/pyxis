cmake_minimum_required(VERSION 3.18)

project(SciCam VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(SciCam
    main.cpp setup.cpp group_delay.cpp 
)

target_include_directories(SciCam PUBLIC ../libs/camera/include ../libs/Eigen include ~/Downloads/build/include)
target_compile_features(SciCam PUBLIC cxx_std_17)

find_package(commander REQUIRED)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

target_link_libraries(SciCam PUBLIC
    commander::commander
    qhyccd
    cfitsio
    pthread
   ${OpenCV_LIBS}
)
