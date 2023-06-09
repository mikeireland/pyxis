cmake_minimum_required(VERSION 3.18)

project(TargetServer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

add_executable(ZaberServer
    main.cpp
)

target_compile_features(ZaberServer PUBLIC cxx_std_17)
target_include_directories(ZaberServer PUBLIC ../libs/camera/include)

find_package(commander REQUIRED)

target_link_libraries(ZaberServer PUBLIC
    commander::commander
)
