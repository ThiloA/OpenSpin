cmake_minimum_required(VERSION 3.5)

project(OpenSpinFork LANGUAGES CXX)
include_directories(.)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(OpenSpinFork main.cpp)
