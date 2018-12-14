cmake_minimum_required(VERSION 3.2)

project(lit)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -O3 -flto -std=c99 -Wno-switch -Wno-unused-parameter -Wno-unused-function -Wno-sequence-point -Wno-unused-variable")
set(CMAKE_C_FLAGS_DEBUG "-g")

file(GLOB_RECURSE SOURCE_FILES src/*.c src/cli/*.c src/vm/*.c src/compiler/*.c src/util/*.c)
include_directories(include/)
add_executable(lit ${SOURCE_FILES})
target_link_libraries(lit m) # Lib math