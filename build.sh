#!/usr/bin/env bash
set -euo pipefail

mkdir -p bin output_files

CXX="${CXX:-c++}"
# -I . so the wrapper includes resolve
"$CXX" -std=c++17 -O2 -Wall -Wextra -I . \
  Interrupts_101268686_101311227.cpp \
  -o bin/interrupts

./bin/interrupts input_files/trace.txt vector_table.txt device_table.txt external_files.txt

sed -n '1,200p' output_files/execution.txt
sed -n '1,120p' output_files/system_status.txt
