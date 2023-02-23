cmake_minimum_required(VERSION 3.18)

project(commander VERSION 0.1.0 LANGUAGES CXX)

add_library(commander
    src/registry.cpp
    src/server.cpp
    src/server/interactive.cpp
    src/server/single_command.cpp
    src/server/socket.cpp
    # include/commander/commander.h
)

target_include_directories(commander PUBLIC include)

target_compile_features(commander PUBLIC cxx_std_17)

find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)
find_package(cppzmq REQUIRED)

target_link_libraries(commander PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    Boost::boost
    cppzmq
)

install(TARGETS commander
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/commander DESTINATION include)
