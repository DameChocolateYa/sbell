cmake_minimum_required(VERSION 3.10)

project(sbell LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set( SOURCES
    src/conffile.cpp
    src/translator.cpp
    src/sbell.cpp)

include_directories(include)

add_executable(sbell ${SOURCES})



