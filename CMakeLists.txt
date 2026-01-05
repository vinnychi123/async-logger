cmake_minimum_required(VERSION 3.15)
project(AsyncLogger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# Include directories
include_directories(include)

# Test executable
add_executable(ring_buffer_test tests/ring_buffer_test.cpp)

# Multi-threaded test executable
add_executable(ring_buffer_mt_test tests/ring_buffer_mt_test.cpp)
target_link_libraries(ring_buffer_mt_test pthread)