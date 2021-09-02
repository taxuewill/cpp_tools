cmake_minimum_required(VERSION 3.0)

project(cpp_tools)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -pthread)

include_directories(
        include
)

add_executable(main
        src/main.cpp
        src/bs_tools/handler.cpp
        )