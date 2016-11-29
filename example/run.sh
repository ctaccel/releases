#!/bin/bash -eu

source /usr/accel/bin/environment.sh

make clean
make

./main input.jpg output.webp

