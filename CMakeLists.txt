# Author: enzo418
# Description: Simple rewrite of the Makefile present here. Exports the
#              uSockets without SSL library as "usocketslib".
# Please include it in your project using add_subdirectory(pathtothisfolder)
# ----
cmake_minimum_required(VERSION 3.12)

option(BUILD_EXAMPLES "Build examples" OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples")

project(uWebSockets)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(src/uWebSockets)
include_directories(uSockets/src)

# it's a header library, it solely depends on uSockets
add_subdirectory(uSockets)

if(BUILD_EXAMPLES)
    file(
        GLOB uwb_example_files 
        "benchmarks/*.cpp"
        "examples/*.cpp")

    foreach(filename ${uwb_example_files})
        STRING(REGEX REPLACE
        "([/\\][A-Za-z0-9_]+)+[/\\]([A-Za-z0-9_]+)\.cpp"
        "\\2" uwb_exec_name ${filename}) # get exec output name
    
        add_executable(${uwb_exec_name} ${filename})
        target_link_libraries(${uwb_exec_name} usocketslib "-lz")
    endforeach()
endif()

# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
