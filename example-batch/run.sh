#!/bin/bash -eu

source /usr/accel/bin/environment.sh

make clean
make

export OMP_NUM_THREADS=24
echo "Using ${OMP_NUM_THREADS} threads"

time ./main commands.cfg

