# Author: enzo418
# Description: Simple rewrite of the Makefile present here. Exports the
#              uSockets without SSL library as "usocketslib".
# ----
cmake_minimum_required(VERSION 3.12)

option(BUILD_EXAMPLES "Build examples" OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples")

project(uWebSockets)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(${CMAKE_CURRENT_LIST_DIR}/src/uWebSockets)

# it's a header library, it solely depends on uSockets
include(${CMAKE_CURRENT_LIST_DIR}/uSockets/CMakeLists.txt)

if(BUILD_EXAMPLES)
    file(
        GLOB uwb_example_files 
        "${CMAKE_CURRENT_LIST_DIR}/benchmarks/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/examples/*.cpp")

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
