cmake_minimum_required(VERSION 2.8)

# Limit build configurations to Debug and Release
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)
endif()
project(algo)

# Mysterious macro to remove default VS "Source Files" and "Header Files" filters.
MACRO (cmp_IDE_SOURCE_PROPERTIES SOURCE_PATH HEADERS SOURCES)
    STRING(REPLACE "/" "\\\\" source_group_path ${SOURCE_PATH}  )
    source_group(${source_group_path} FILES ${HEADERS} ${SOURCES})
ENDMACRO (cmp_IDE_SOURCE_PROPERTIES NAME HEADERS SOURCES INSTALL_FILES)

set(CMAKE_BUILD_TYPE Debug)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
SET(headers algo.h test_common.h)

set(exe_name test_allocpool)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})

set(exe_name test_heap)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})

set(exe_name test_queue)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})

set(exe_name test_stack)
set(sources ${exe_name}.c)
cmp_IDE_SOURCE_PROPERTIES( "\\\\" "${headers}" "${sources}")
add_executable(${exe_name} ${sources} ${headers})
