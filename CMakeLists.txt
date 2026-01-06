cmake_minimum_required(VERSION 3.15)
project(AsyncLogger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# Include directories
include_directories(include)

# Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests")
add_subdirectory(external/benchmark)

# Logger library
add_library(logger src/logger.cpp)
target_link_libraries(logger pthread)

# Test executables
add_executable(ring_buffer_test tests/ring_buffer_test.cpp)

add_executable(ring_buffer_mt_test tests/ring_buffer_mt_test.cpp)
target_link_libraries(ring_buffer_mt_test pthread)

add_executable(logger_test tests/logger_test.cpp)
target_link_libraries(logger_test logger)

# Benchmark executables
add_executable(ring_buffer_benchmark benchmarks/ring_buffer_benchmark.cpp)
target_link_libraries(ring_buffer_benchmark benchmark::benchmark pthread)

add_executable(ring_buffer_benchmark_realistic benchmarks/ring_buffer_benchmark_realistic.cpp)
target_link_libraries(ring_buffer_benchmark_realistic benchmark::benchmark pthread)

add_executable(ring_buffer_latency benchmarks/ring_buffer_latency.cpp)
target_link_libraries(ring_buffer_latency pthread)

