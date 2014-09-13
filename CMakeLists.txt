cmake_minimum_required(VERSION 2.8)
project(algo)

set(CMAKE_BUILD_TYPE Debug)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(exe_name test_heap)
set(sources algo.h test_common.h ${exe_name}.c)
add_executable(${exe_name} ${sources})

set(exe_name test_queue)
set(sources algo.h test_common.h ${exe_name}.c)
add_executable(${exe_name} ${sources})

set(exe_name test_stack)
set(sources algo.h test_common.h ${exe_name}.c)
add_executable(${exe_name} ${sources})
