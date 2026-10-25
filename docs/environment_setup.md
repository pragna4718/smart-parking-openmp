# Phase 3: Environment Setup & Hardware Specification

## 1. Overview
This document records the hardware environment, compiler toolchain, runtime dependencies, build flags, and verification test results for the OpenMP Parallel Smart Parking Allocation project.

---

## 2. Machine Hardware & Operating System Specifications

The benchmarking and development platform for this project is an Apple Silicon workstation.

| System Specification Parameter | Measured System Value | Architectural Significance |
| :--- | :--- | :--- |
| **Workstation Model** | Mac15,12 | Apple Silicon SoC platform |
| **Operating System** | macOS 26.5.2 (Darwin arm64) | 64-bit Unix environment |
| **Processor (CPU)** | **Apple M3** | Heterogeneous ARMv8.5-A architecture |
| **Physical Core Count** | **8 Cores** | 4 Performance (Firestorm/P) + 4 Efficiency (Icestorm/E) cores |
| **Logical Thread Count** | **8 Concurrent Threads** | 1 thread per core (no hyper-threading) |
| **Unified System Memory (RAM)** | **8.00 GB** ($8{,}589{,}934{,}592$ bytes) | Shared high-bandwidth LPDDR5 architecture |
| **Memory Bus Width** | 128-bit | Unified memory accessible with low latency by CPU and cache |
| **L1 / L2 Cache Architecture** | P-cores: 192KB L1 I-cache, 128KB L1 D-cache, 16MB shared L2<br>E-cores: 128KB L1 I-cache, 64KB L1 D-cache, 4MB shared L2 | Critical cache size guiding SoA vector layout |

---

## 3. Compiler & OpenMP Toolchain Setup

### 3.1 Compiler Selection
- **Default System Compiler**: Apple Clang version 21.0.0 (`clang-2100.1.1.101`).
- **OpenMP Runtime Library**: LLVM `libomp` 23.1.4 (installed via Homebrew at `/opt/homebrew/opt/libomp`).

### 3.2 Compilation & Linker Flags
Apple Clang requires delegating OpenMP pragma parsing to the preprocessor via `-Xpreprocessor` and linking against the LLVM OpenMP runtime library `-lomp`:

| Flag | Parameter / Path | Purpose |
| :--- | :--- | :--- |
| **Language Standard** | `-std=c++17` | Enables structured bindings, `std::atomic` conveniences, and filesystem. |
| **Optimization Level** | `-O2` | Identical optimization level used across serial and parallel builds for fair benchmarking. |
| **Preprocessor Include** | `-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include` | Directs Clang to parse OpenMP pragmas and load `omp.h`. |
| **Linker Library** | `-L/opt/homebrew/opt/libomp/lib -lomp` | Links the LLVM OpenMP multi-threaded runtime engine. |
| **Diagnostics** | `-Wall -Wextra` | Enforces rigorous code quality and safety warnings. |

On Linux / GCC environments, the build system automatically switches to `-fopenmp` for both compilation and linking.

---

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
