cmake_minimum_required(VERSION 3.18)

project(FST_Test VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(FST_Test
 #   ../libs/imageproc/src/image.cpp ../libs/lost/centroiders.cpp ../plate_solver/astrometry/util/simplexy.c test_centroid.cpp 
    ../libs/imageproc/src/image.cpp test_centroid.cpp 
)

#target_include_directories(FST_Test PUBLIC  ../libs/lost ../libs/imageproc/include ../plate_solver/astrometry/include/astrometry ../plate_solver/astrometry/include)
target_include_directories(FST_Test PUBLIC  ../libs/imageproc/include )
target_compile_features(FST_Test PUBLIC cxx_std_17)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

target_link_libraries(FST_Test PUBLIC
    cfitsio
   ${OpenCV_LIBS}
)
