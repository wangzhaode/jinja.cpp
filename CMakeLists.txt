cmake_minimum_required(VERSION 3.10)
project(jinja_cpp VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only Library
add_library(jinja INTERFACE)
target_include_directories(jinja INTERFACE . third_party)

# Test
enable_testing()
add_executable(test_main tests/test_main.cpp)
target_link_libraries(test_main jinja)
