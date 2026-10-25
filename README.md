# Parallel Smart Parking Allocation Engine (OpenMP)

A high-performance concurrent parking slot allocation engine implemented in C++17 using OpenMP. The system efficiently assigns a large batch of heterogeneous vehicle requests across multi-floor parking facilities according to a multi-objective cost function while preventing double-bookings.

---

## 1. Project Roadmap & Completion Status

- [x] **Phase 1: Requirements & Test Scenarios Specification**
  - Multi-floor facility domain model, vehicle request attributes, compatibility constraints, and stress test scenarios ([`docs/requirements.md`](docs/requirements.md)).
- [x] **Phase 2: System Design**
  - Structure-of-Arrays (SoA) data structures, multi-factor cost formulation, greedy proposal allocation strategy, concurrency race-condition analysis, and algorithmic flow diagram ([`docs/system_design.md`](docs/system_design.md)).
- [x] **Phase 3: Environment Setup & OpenMP Toolchain**
  - LLVM `libomp` integration with Apple Clang on Apple M3, hardware specification table, Makefile automation, and OpenMP verification binary ([`docs/environment_setup.md`](docs/environment_setup.md)).
- [ ] **Phase 4: Input Generation & Serial Baseline Implementation**
  - Deterministic synthetic generator for facilities and vehicle batches, serial baseline solver.
- [ ] **Phase 5: Parallel Implementation & Concurrency Optimization**
  - OpenMP parallel round-based arbitrated proposal engine and optional zone-partitioned variant.
- [ ] **Phase 6: Validation, Benchmarking & Speedup Analysis**
  - Zero double-booking verification across thread counts (1, 2, 4, 8), speedup and efficiency curves.

---

## 2. Hardware Environment

Benchmarked on **Apple M3** (Mac15,12):
- **CPU**: Apple M3 (8 cores: 4 Performance + 4 Efficiency cores)
- **Logical Threads**: 8 concurrent hardware execution threads
- **Memory**: 8.00 GB unified LPDDR5 RAM
- **Compiler**: Apple Clang 21.0.0 with LLVM `libomp` 23.1.4
- **Flags**: `-O2 -std=c++17 -Wall -Wextra`

---

## 3. Quickstart & Verification

### Build & Run OpenMP Verification
```bash
# Build and run OpenMP smoke test
make hello

# Test execution with custom thread counts
OMP_NUM_THREADS=4 ./bin/hello_openmp
OMP_NUM_THREADS=8 ./bin/hello_openmp
```

### Clean Build Artifacts
```bash
make clean
```
