# Parallel-Traffic-Simulation

## Introduction
Traffic simulation using Message Passing Interface (MPI).   
Author: Zhan, Kewei | Zhang, Yi   
An USC EE-451 Final Project instructed by <i>Dr. Viktor Prasanna</i>
## Usage
Compile:
```SHELL
cmake . 
make
```
Run:
```SHELL
cd CMakeFiles
mpirun -np 2 mpi_program
```

## References:
Logging library: [rxi/log.c](https://github.com/rxi/log.c)