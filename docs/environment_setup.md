# Phase 3: Environment Setup & Hardware Specification

## 1. Overview
This document records the hardware environment, compiler toolchain, runtime dependencies, build flags, and verification test results for the OpenMP Parallel Smart Parking Allocation project.

---

## 2. Machine Hardware & Operating System Specifications

## 2. Machine Hardware & Operating System Specifications

The benchmarking and development platform is an Apple Silicon workstation.

| System Specification Parameter | Measured System Value | Architectural Significance |
|---|---|---|
| Workstation Model | MacBook Air (Mac16,12) | Apple Silicon platform |
| Processor (CPU) | Apple M4 | ARM-based Apple Silicon architecture |
| Total Core Count | 10 cores | 4 performance cores and 6 efficiency cores |
| Memory (RAM) | 16 GB | Unified system memory |
| Architecture | ARM64 | 64-bit Apple Silicon architecture |
| Operating System | macOS | Unix-based development environment |

## 3. Compiler & OpenMP Toolchain Setup

## 3. Compiler & OpenMP Toolchain Setup

### 3.1 Compiler Selection

- **Compiler:** Apple Clang 17.0.0
- **Target Architecture:** ARM64 (Apple Silicon)
- **OpenMP Runtime Library:** LLVM libomp installed through Homebrew.
- **OpenMP Installation Path:** `/opt/homebrew/opt/libomp`

### 3.2 Compilation & Linker Flags

The project uses the C++17 standard and OpenMP for parallel execution.

The following command is used to compile the project:

```bash
g++ -std=c++17 -O3 -Wall \
-Xpreprocessor -fopenmp \
-I/opt/homebrew/opt/libomp/include \
src/generator.cpp \
src/cost.cpp \
src/validator.cpp \
src/serial_alloc.cpp \
src/parallel_alloc.cpp \
src/main.cpp \
-L/opt/homebrew/opt/libomp/lib \
-lomp \
-o smart_parking
```

### 3.3 Explanation of Compilation Flags

| Flag | Purpose |
|---|---|
| `-std=c++17` | Uses the C++17 language standard. |
| `-O3` | Enables compiler optimizations for improved performance. |
| `-Wall` | Enables common compiler warnings. |
| `-Xpreprocessor -fopenmp` | Enables OpenMP pragma processing. |
| `-I/opt/homebrew/opt/libomp/include` | Specifies the OpenMP header location. |
| `-L/opt/homebrew/opt/libomp/lib` | Specifies the OpenMP library location. |
| `-lomp` | Links the LLVM OpenMP runtime library. |

The generated executable is named `smart_parking`.

On macOS, the Homebrew LLVM OpenMP runtime is used to execute parallel regions.
## 4. Verification & Smoke Test

The OpenMP runtime was verified using a multi-threaded test program (`src/hello_openmp.cpp`).

### 4.1 Verification Code (`src/hello_openmp.cpp`)
```cpp
#include <omp.h>
#include <cstdio>

int main() {
    printf("========================================\n");
    printf("OpenMP Environment Verification\n");
    printf("========================================\n");
    printf("Max Available Threads: %d\n\n", omp_get_max_threads());

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        #pragma omp critical
        {
            printf("[Thread %d of %d] Active on Apple M3 Core\n", tid, nthreads);
        }
    }

    printf("\nOpenMP initialization successfully verified!\n");
    return 0;
}
```

### 4.2 Verified Terminal Output
```
========================================
OpenMP Environment Verification
========================================
Max Available Threads: 8

[Thread 0 of 8] Active on Apple M3 Core
[Thread 4 of 8] Active on Apple M3 Core
[Thread 7 of 8] Active on Apple M3 Core
[Thread 5 of 8] Active on Apple M3 Core
[Thread 2 of 8] Active on Apple M3 Core
[Thread 1 of 8] Active on Apple M3 Core
[Thread 6 of 8] Active on Apple M3 Core
[Thread 3 of 8] Active on Apple M3 Core

OpenMP initialization successfully verified!
```

---

## 5. Build Automation (`Makefile`) Targets

| Command | Action |
| :--- | :--- |
| `make hello` | Compiles and executes the OpenMP verification test binary. |
| `make all` | Builds all executables (serial, parallel, benchmarks). |
| `make clean` | Removes compiled binaries, object files, and temporary build artifacts. |
| `OMP_NUM_THREADS=N ./bin/hello_openmp` | Runs the binary with an explicit thread count $N$. |
