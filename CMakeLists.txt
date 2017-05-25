cmake_minimum_required(VERSION 3.6)
project(leveldb)

set(CMAKE_CXX_STANDARD 11)

SET(GCC_COVERAGE_COMPILE_FLAGS "-lgtest")
SET(GCC_COVERAGE_LINK_FLAGS    "-lgtest")



set(SOURCE_FILES main.cpp)
add_executable(leveldb ${SOURCE_FILES})