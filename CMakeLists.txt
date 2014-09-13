cmake_minimum_required(VERSION 2.8)
project(algo)

set(CMAKE_BUILD_TYPE Debug)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(exe_name test_algo)
set(sources algo.h test_algo.c)
add_executable(${exe_name} ${sources})
