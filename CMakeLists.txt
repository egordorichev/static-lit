cmake_minimum_required(VERSION 3.2)
project(lit)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES cli/*.cpp vm/src/*.cpp)
add_executable(lit ${SOURCE_FILES} vm/include/lit_value.hpp vm/include/lit_value.hpp vm/include/lit_value.hpp vm/include/lit_common.hpp vm/src/lit_value.cpp vm/src/lit_common.cpp)

include_directories(vm/include/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")