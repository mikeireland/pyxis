cmake_minimum_required(VERSION 3.18)
project(toml LANGUAGES CXX)

add_library (toml toml.cpp)
target_compile_definitions(toml PUBLIC TOML_HEADER_ONLY=0)
target_include_directories (toml PUBLIC ./)