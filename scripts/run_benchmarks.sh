#!/usr/bin/env bash
# ==============================================================================
# Script: run_benchmarks.sh
# Purpose: Executes OpenMP scaling benchmarks across multiple thread counts,
#          generates CSV performance metrics, and triggers plot generation.
# ==============================================================================

set -e

echo "=== [1/3] Building Project with Optimizations (-O3, -fopenmp) ==="
if command -v mingw32-make >/dev/null 2>&1; then
    mingw32-make all
elif command -v make >/dev/null 2>&1; then
    make all
else
    g++ -std=c++17 -O3 -Wall -fopenmp \
        src/generator.cpp \
        src/cost.cpp \
        src/validator.cpp \
        src/serial_alloc.cpp \
        src/parallel_alloc.cpp \
        src/main.cpp \
        -o smart_parking.exe
fi

EXECUTABLE="./smart_parking.exe"
if [ ! -f "$EXECUTABLE" ]; then
    EXECUTABLE="./smart_parking"
fi

echo ""
echo "=== [2/3] Running OpenMP Scaling Benchmark ==="
# Problem Size: 5 floors x 40 rows x 50 cols = 10,000 slots, 8,000 vehicle requests
$EXECUTABLE --floors 5 --rows 40 --cols 50 --vehicles 8000 --benchmark --csv benchmark_results.csv

echo ""
echo "=== [3/3] Generating Performance Graphs ==="
if command -v python >/dev/null 2>&1; then
    python scripts/plot_results.py
elif command -v python3 >/dev/null 2>&1; then
    python3 scripts/plot_results.py
else
    echo "Warning: Python not found. Skipping plot generation."
fi

echo ""
echo "=== BENCHMARKING COMPLETED SUCCESSFULLY ==="
