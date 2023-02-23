cmake_minimum_required(VERSION 3.18)

project(example_cli VERSION 0.1.0 LANGUAGES CXX)

add_executable(example_cli
    src/example.cpp
    src/gyro.cpp
)
target_include_directories(example_cli PUBLIC include)

target_compile_features(example_cli PUBLIC cxx_std_17)

find_package(commander REQUIRED)

target_link_libraries(example_cli PUBLIC
    commander::commander
)
