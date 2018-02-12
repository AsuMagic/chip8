cmake_minimum_required(VERSION 2.8)

project(chip8)
add_executable(${PROJECT_NAME} "main.cpp")

set(CMAKE_CXX_FLAGS "-std=c++1z -Wall -Wextra -pedantic-errors")

target_link_libraries(chip8 sfml-system sfml-graphics sfml-window)
