#!/usr/bin/bash

NUM_ITER=1000

rm runs.txt

for ((i=1; i<=NUM_ITER; i++)); do
    echo "${i}/${NUM_ITER}"
    ./build/ladybugger | awk '/Thread 0:/ {print $3}' |  tr '\n' '\t' |  tr 'ns' ' ' >> runs.txt
    echo "" >> runs.txt
done
