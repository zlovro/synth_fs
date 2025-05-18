cmake_minimum_required(VERSION 3.28)
project(synth_fs C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)
set(CMAKE_VERBOSE_MAKEFILE)

include_directories(.)
include_directories(lib)
include_directories(lib/synthinf)

add_executable(synth_fs
        main.cpp
        lib/solfege/solfege.c
        lib/sfs/synth_fs.c
        lib/sfs/synth_fs.h
        lib/synthinf/synth_errno.h
        lib/synthinf/synth_errno.c
        loop_detection.cpp
        loop_detection.h
        lib/json/json.hpp
        main.cpp
)