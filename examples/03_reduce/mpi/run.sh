#!/bin/bash

echo number of process = 4, msg = 7
/home/evalen/devtools/bin/mpirun -n 4 ./mpi_ring 7

echo number of process = 7, msg = 4
/home/evalen/devtools/bin/mpirun -n 7 ./mpi_ring 4
