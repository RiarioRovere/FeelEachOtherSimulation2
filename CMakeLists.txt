cmake_minimum_required(VERSION 3.21)
project(FeelEachOtherSimulation)

set(CMAKE_CXX_STANDARD 23)

add_executable(FeelEachOtherSimulation main.cpp radio_channel.h locket.h configuration.h engine.h)

add_subdirectory(tests)
include_directories(tests)

find_package (glog 0.7.0 REQUIRED)

target_link_libraries (FeelEachOtherSimulation glog::glog)
