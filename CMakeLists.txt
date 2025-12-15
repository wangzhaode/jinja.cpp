cmake_minimum_required(VERSION 3.10)
project(jinja_cpp VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(third_party)

# Library
add_library(jinja src/jinja.cpp)

# Test
enable_testing()
add_executable(test_main tests/test_main.cpp)
target_link_libraries(test_main jinja)
