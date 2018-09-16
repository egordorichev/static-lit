cmake_minimum_required(VERSION 3.2)
project(lit)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE SOURCE_FILES vm/src/*.cpp)
add_executable(lit ${SOURCE_FILES})

include_directories(vm/include/)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC")