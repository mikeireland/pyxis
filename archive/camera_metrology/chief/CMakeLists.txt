cmake_minimum_required (VERSION 3.18)
project(chief)

find_package(fmt REQUIRED)

set(helpstring_file
    help.toml
    cmd_elseif.include
    helpstring.include
)

set(helpstring_dest ${helpstring_file})
list(TRANSFORM helpstring_dest PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_custom_command(
    OUTPUT
    ${helpstring_dest}
    COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/util/extract_commands.py
    COMMAND
    mv ${helpstring_file} ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DEPENDS
    src/server.cpp
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

add_custom_target(helpstring
    DEPENDS ${helpstring_dest}
)

add_subdirectory(src/toml++)
add_subdirectory(src/imageproc)

add_library(server
    src/server.cpp
    src/serverfunc.cpp
    ${helpstring_dest}
)

target_link_libraries(server
    PUBLIC
    toml
    PRIVATE
    zmq
    fmt::fmt
    image
)

target_include_directories(server
    PUBLIC
    include
)

add_executable(main src/main.cpp)
target_link_libraries(main server)