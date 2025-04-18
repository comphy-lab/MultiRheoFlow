#!/bin/bash

# Save the original directory
ORIG_DIR=$(pwd)

mkdir -p $1

cp $1.c $1/
cd $1

qcc -I${ORIG_DIR}/src-local -I${ORIG_DIR}/../src-local -O2 -Wall -disable-dimensions $1.c -o $1 -lm
./$1