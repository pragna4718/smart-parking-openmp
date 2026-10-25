#!/usr/bin/env bash
# ==============================================================================
# Script: run_tests.sh
# Purpose: Compiles the Parallel Smart Parking Allocation project and runs the
#          comprehensive test suite covering all functional scenarios.
# ==============================================================================

set -e

echo "=== [1/2] Building Parallel Smart Parking Allocation Project ==="
if command -v mingw32-make >/dev/null 2>&1; then
    mingw32-make all
elif command -v make >/dev/null 2>&1; then
    make all
else
    echo "Using direct g++ compilation..."
    g++ -std=c++17 -O3 -Wall -fopenmp \
        src/generator.cpp \
        src/cost.cpp \
        src/validator.cpp \
        src/serial_alloc.cpp \
        src/parallel_alloc.cpp \
        src/main.cpp \
        -o smart_parking.exe
fi

echo ""
echo "=== [2/2] Running Correctness & Invariant Test Suite ==="
if [ -f "./smart_parking.exe" ]; then
    ./smart_parking.exe --test
elif [ -f "./smart_parking" ]; then
    ./smart_parking --test
else
    echo "Error: smart_parking executable not found!"
    exit 1
fi

echo ""
echo "=== ALL TESTS COMPLETED SUCCESSFULLY ==="
