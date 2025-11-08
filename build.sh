#!/usr/bin/env bash
set -euo pipefail

# fresh dirs
mkdir -p bin output_files

# build
CXX="${CXX:-c++}"
"$CXX" -std=c++17 -O2 -Wall -Wextra interrupts_101311227_101268686.cpp -o bin/interrupts

# run the simulator on the main test inputs
./bin/interrupts input_files/trace.txt vector_table.txt device_table.txt external_files.txt

# show a quick peek of outputs
sed -n '1,200p' output_files/execution.txt
sed -n '1,120p' output_files/system_status.txt
