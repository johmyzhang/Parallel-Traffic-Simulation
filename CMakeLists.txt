cmake_minimum_required(VERSION 3.29)
project(Parallel_Traffic_Simulation C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Parallel_Traffic_Simulation
    entity.h
    map.c
        route.h
        route.c
        globals.h
        entity.c
)
