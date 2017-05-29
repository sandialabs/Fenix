#!/bin/bash

# If you keep FENIX_SRC as '.', you can cd into the root of Fenix
# sources and run './cmake_scripts/run_cmake.sh'

FENIX_SRC=.
ULFM_HOME=<Your ULFM installation directory>

cmake ${FENIX_SRC} \
    -DCMAKE_C_COMPILER:STRING=${ULFM_HOME}/bin/mpicc \
    -DCMAKE_CXX_COMPILER:STRING=${ULFM_HOME}/bin/mpic++
