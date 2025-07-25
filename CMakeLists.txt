cmake_minimum_required(VERSION 3.28)
project(synth_fs C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)
set(CMAKE_VERBOSE_MAKEFILE)

include_directories(.)
include_directories(lib)
include_directories(lib/synthinf)

add_executable(synth_fs
        lib/solfege/solfege.c
        lib/json/json.hpp
        lib/wav/wav.c
        lib/synthinf/serrno.c
        main.cpp
)