cmake_minimum_required(VERSION 3.29)
project(Parallel_Traffic_Simulation C)

set(CMAKE_C_STANDARD 11)


find_package(MPI REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.c)

add_executable(mpi_program ${SOURCES})

target_link_libraries(mpi_program MPI::MPI_C)