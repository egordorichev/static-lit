cmake_minimum_required(VERSION 3.2)

project(lit)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # require at least gcc 4.9
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        message(FATAL_ERROR "GCC version must be at least 4.9.0!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with GCC.")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -O3 -flto -std=c99 -Wno-switch -Wno-unused-parameter -Wno-unused-function -Wno-sequence-point -Wno-unused-variable -Wno-unused-label")
set(CMAKE_C_FLAGS_DEBUG "-g")

file(GLOB_RECURSE SOURCE_FILES src/*.c src/cli/*.c src/vm/*.c src/compiler/*.c src/util/*.c)
include_directories(include/)
add_executable(lit ${SOURCE_FILES})
target_link_libraries(lit m) # Lib math
