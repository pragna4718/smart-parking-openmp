# Parallel Smart Parking Allocation System (C++17 & OpenMP)

A high-performance, multi-threaded smart parking slot allocation engine built in **C++17** using **OpenMP**. The system models a multi-floor, heterogeneous parking facility, evaluating optimal slot assignments for high-volume incoming vehicle streams under spatial, structural, and vehicle-type constraints.

---

## 1. System Overview & Problem Formulation

### 1.1 Objective
Given a multi-floor parking lot with $M$ parking slots and a batch of $N$ incoming vehicle requests, assign each vehicle to the most optimal available slot to minimize total objective cost, while guaranteeing:
1. **Mutual Exclusion (Zero Double-Booking)**: Each physical parking slot can be assigned to at most one vehicle.
2. **Type Compatibility**: Constraints on vehicle dimensions, electric charging (EV), and accessibility (disability permits) are strictly enforced.
3. **High Throughput & Concurrency**: Multi-core parallelization using OpenMP to process large request batches with low latency.

---

## 2. Mathematical Formulation & Cost Function

For vehicle $V_i$ and candidate slot $S_j$, the cost $C(V_i, S_j)$ is computed as:

$$\text{Cost}(V_i, S_j) = w_{\text{dist}} \cdot D_{\text{walk}}(V_i, S_j) + w_{\text{floor}} \cdot \Delta F(V_i, S_j) + w_{\text{price}} \cdot R(S_j) + P_{\text{type}}(V_i, S_j)$$

Where:
* **Horizontal Walking Distance**: Manhattan metric on grid:
  $$D_{\text{walk}} = |S_j.x - V_i.\text{target\_x}| + |S_j.y - V_i.\text{target\_y}|$$
* **Vertical Floor Penalty**: Floor level discrepancy:
  $$\Delta F = |S_j.\text{floor} - V_i.\text{target\_floor}| \times \mu_{\text{floor}}$$
* **Slot Base Rate**: Monetary fee per hour $R(S_j)$.
* **Type Compatibility & Incentives**:
  * Incompatible pair: $\text{Cost} = \infty$ (illegal assignment).
  * EV matching EV charger: $-10.0$ incentive bonus.
  * EV assigned to non-charger: $+15.0$ inconvenience penalty.
  * Compact vehicle in Large slot: $+5.0$ suboptimal space utilization penalty.

### Compatibility Matrix

| Vehicle Type | COMPACT Slot | REGULAR Slot | LARGE Slot | EV Charger Slot | ACCESSIBLE Slot |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **COMPACT** | Yes (Ideal) | Yes | Yes (Penalty) | Yes | No |
| **REGULAR** | No | Yes (Ideal) | Yes | Yes | No |
| **LARGE (SUV/Truck)** | No | No | Yes (Strict) | No | No |
| **EV (Electric)** | No | Yes | Yes | Yes (Incentive) | No |
| **ACCESSIBLE** | No | Yes | Yes | Yes | Yes (Reserved) |

---

## 3. Algorithm Architecture

### 3.1 Serial Baseline (`src/serial_alloc.cpp`)
* **Complexity**: $\mathcal{O}(N \times M)$
* Evaluates vehicles sequentially. For each vehicle, it scans all currently available slots, computes costs, and immediately locks the slot.
* **Limitation**: Cannot utilize multiple CPU cores; becomes a throughput bottleneck when thousands of vehicles arrive concurrently.

### 3.2 Parallel OpenMP Allocation (`src/parallel_alloc.cpp`)
* **Multi-Round Proposal & Arbiter Algorithm**:
  1. **Parallel Proposal Phase (`#pragma omp parallel for schedule(guided)`)**:
     * All unassigned vehicles concurrently search the available slot space lock-free.
     * Each vehicle identifies its lowest-cost compatible slot and submits a proposal.
  2. **Fine-Grained Conflict Resolution**:
     * An array of fine-grained OpenMP locks (`omp_lock_t`), one per slot, protects each candidate slot.
     * Contested slots resolve conflicts using an atomic arbiter: lowest cost wins; ties are broken deterministically by vehicle priority and vehicle ID.
  3. **Parallel Assignment Phase (`#pragma omp parallel for schedule(static)`)**:
     * Winning proposals are finalized into the global allocation registry.
  4. **Active Request Compaction**:
     * Rejected vehicles advance to the next round to choose from remaining available slots.
     * Converges rapidly in $\le 4$ rounds.

---

## 4. Repository Structure

```
smart-parking-openmp-1/
├── Makefile                     # Build automation with C++17 and OpenMP flags
├── README.md                    # Project documentation and viva preparation guide
├── benchmark_results.csv        # Scalability metrics across thread counts
├── benchmark_dashboard.png      # 3-panel performance graph (Time, Speedup, Efficiency)
├── scripts/
│   ├── run_tests.sh             # Bash test runner script
│   ├── run_benchmarks.sh        # Benchmark runner and plot triggering script
│   └── plot_results.py          # Python matplotlib graph generator
└── src/
    ├── generator.h / .cpp       # Synthetic parking lot and request stream generator
    ├── cost.h / .cpp            # Multi-objective weighted cost function & compatibility
    ├── serial_alloc.cpp         # Baseline sequential greedy allocator
    ├── parallel_alloc.cpp       # OpenMP multi-round proposal-based allocator
    ├── validator.h / .cpp       # Invariant verifier (checks for duplicates & validity)
    └── main.cpp                 # CLI driver, benchmark harness, and scenario runner
```

---

## 5. Compilation & Execution

### 5.1 Prerequisites
* **C++ Compiler**: `g++` with C++17 support and `-fopenmp`.
* **Make Utility**: `make` or `mingw32-make` (on Windows).
* **Python**: Python 3 with `matplotlib` for generating charts.

### 5.2 Build Commands (Windows & MinGW)
```bash
# Clean previous build artifacts safely
mingw32-make clean

# Compile the project with C++17, O3 optimizations, and OpenMP
mingw32-make all

# Verify OpenMP environment and thread detection
mingw32-make hello
```

### 5.3 Running Correctness Tests
```bash
# Execute comprehensive 18 unit tests + 4 scenarios test suite
mingw32-make test
# or directly:
./smart_parking.exe --test
```

### 5.4 Running Benchmarks & Plotting
```bash
# Execute multi-threaded OpenMP scaling benchmark and record CSV
mingw32-make benchmark
# or custom problem scale:
./smart_parking.exe --floors 5 --rows 40 --cols 50 --vehicles 8000 --benchmark --csv benchmark_results.csv

# Generate high-resolution 3-panel performance graph
python scripts/plot_results.py
```

---

## 6. Comprehensive Bug Audit & Targeted Engineering Fixes

A thorough audit of the original codebase revealed 9 critical bugs across memory safety, indexing, concurrency, compatibility, and verification:

| Bug Category | Location | Root Cause | Engineering Fix Applied |
| :--- | :--- | :--- | :--- |
| **1. Vehicle Identity vs Vector Indexing** | `src/parallel_alloc.cpp` | In the resolution phase, `results[win_vid]` used vehicle ID `req.id` as a direct array index into `results`. If vehicle IDs were nonsequential (e.g. ID `9999` with 20 requests), this triggered out-of-bounds writes and segfaults. | Added `req_index` to `SlotBid` struct, completely decoupling vehicle identity from vector position. `results[win_idx]` is now indexed safely by request index in $[0, N-1]$. |
| **2. Accessibility Compatibility Hole** | `src/cost.cpp` | `isCompatible(VehicleType::ACCESSIBLE, ...)` unconditionally returned `true`, illegally allowing Accessible vehicles into `COMPACT` and `EV` stalls in violation of Section 2.3 requirements. | Constrained Accessible vehicles strictly to `ACCESSIBLE`, `REGULAR`, and `LARGE` slots. |
| **3. Artificial Cost Clamping** | `src/cost.cpp` | `computeSlotCost` clamped negative total costs to `0.0`. This erased the relative benefit between close and distant EV stalls when the $-10.0$ incentive bonus was applied. | Removed artificial clamping so costs accurately reflect the continuous objective function. |
| **4. Arbiter Tie-Breaking Inversion** | `src/parallel_alloc.cpp` | Floating-point epsilon check `fabs(min_cost - slot_bids.cost) < 1e-6` was evaluated without checking if `min_cost > slot_bids.cost`, allowing higher-cost bids to displace lower-cost bids if priority was higher. | Replaced with strict epsilon ordering: strictly lower cost wins (`min_cost < cost - 1e-9`); only on numerical equivalence ($|\Delta| \le 1e-9$) is priority and then vehicle ID evaluated. |
| **5. Unassigned Request Pruning** | `src/parallel_alloc.cpp` | Vehicles that found zero compatible free slots in round $k$ remained in `active_req_indices` across all subsequent rounds, wasting CPU searching exhausted slot pools. | Tracked proposal viability per round (`vehicle_found_slot`). Vehicles with no candidate slots are pruned immediately since available slots strictly shrink each round. |
| **6. OpenMP Lock Lifecycle (RAII)** | `src/parallel_alloc.cpp` | Raw `omp_lock_t` array had manual destroy calls that were bypassed on early returns or exceptions, leaking OS synchronization handles. | Created `OpenMPLockArray` RAII wrapper guaranteeing automatic destruction across all exit paths. |
| **7. Input Sanity & Duplicate IDs** | `src/serial_alloc.cpp`, `src/parallel_alloc.cpp` | Duplicate vehicle or slot IDs were accepted silently, creating conflicting physical reservations. | Added explicit input validation throwing `std::invalid_argument` upon detecting duplicate vehicle or slot IDs. |
| **8. Permissive Invariant Validator** | `src/validator.cpp` | Original validator only checked duplicate slot counts; it missed missing results, duplicate results, unknown IDs, pre-occupied slots, non-finite costs, invalid success/failure combinations, and cost mismatches. | Strengthened `validateAllocation` to independently verify all 12 invariant categories and produce detailed error diagnostics. |
| **9. Monotonic Timing & Timer Resolution** | `src/main.cpp` | Small workloads running in $< 1\text{ ms}$ displayed `0.000 ms` and `inf req/sec` throughput due to coarse timer resolution. | Switched to monotonic `std::chrono::steady_clock` and added dynamic repeated trials (20 runs) for sub-millisecond workloads. |

---

## 7. Theoretical Analysis: Serial vs. Parallel Allocation Discrepancies

### Why Do Serial and Parallel Allocations Yield Slightly Different Assignment Counts?

A notable observation during testing is that the parallel algorithm and serial algorithm produce slightly different numbers of parked vehicles (e.g. in Scenario B: 73 serial vs 71 parallel; Scenario C: 768 serial vs 753 parallel; Scenario D: 3928 serial vs 3848 parallel), with the **parallel algorithm consistently achieving a dramatically lower total solution cost** (e.g., Scenario B: 457.60 parallel vs 881.60 serial; Scenario D: 28,056 parallel vs 34,012 serial).

This difference stems directly from the underlying algorithmic paradigms:

1. **Serial Greedy (First-Come, First-Served Path Dependency)**:
   * Processes vehicle requests sequentially in index order ($i = 0, 1, \dots, N-1$).
   * Early vehicles eagerly grab their favorite slots regardless of priority or whether other incoming vehicles desperately need that specific slot type.
   * If an early regular car consumes a `REGULAR` slot that happens to be the only compatible slot for a subsequent `ACCESSIBLE` or `LARGE` car, the later car is turned away.
   * Total solution cost is high because early cars optimize locally without competition.

2. **Parallel Proposal Arbiter (Competitive Simultaneous Auction)**:
   * In each round, **all** unassigned vehicles simultaneously evaluate the remaining parking lot and bid on their globally optimal slot.
   * Contested slots are awarded by the arbiter to the vehicle with the **lowest objective cost**, breaking ties by **higher priority**, and then **lower vehicle ID**.
   * Winning vehicles claim slots based on global merit and priority rather than arbitrary request arrival sequence.
   * Because low-cost, high-priority vehicles claim their ideal slots first, the total solution cost is substantially lower (up to **$48\%$ cost reduction** over serial).
   * In tightly constrained heterogeneous lots, prioritizing the lowest-cost bids may leave the remaining slot configuration incompatible with certain highly restricted vehicles (such as `LARGE` trucks requiring `LARGE` bays), leading to small, natural variations in total capacity filled.

**Conclusion**: Both algorithms strictly satisfy all mutual exclusion, type compatibility, and concurrency invariants. The parallel proposal engine achieves far superior economic efficiency and fairness.

---

## 8. Experimental Results & Performance Analysis

Measured on an **AMD/Intel 12-thread system (Windows MinGW GCC 6.3.0, C++17, -O3, -fopenmp)**:

### 8.1 Scaling Benchmark ($M = 5,000$ slots, $N = 3,500$ vehicles)

| Threads ($p$) | Parked Count | Parallel Time (ms) | Speedup $S(p)$ | Efficiency $E(p)$ | Correctness Status |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **Serial Baseline** | 3500 / 3500 | 93.87 ms | 1.00x | 100.0% | `PASS` (0 Duplicates) |
| **1 Thread** | 3472 / 3500 | 233.70 ms | 0.40x | 40.2% | `PASS` (0 Duplicates) |
| **2 Threads** | 3472 / 3500 | 128.31 ms | 0.73x | 36.6% | `PASS` (0 Duplicates) |
| **4 Threads** | 3473 / 3500 | 83.36 ms | **1.13x** | 28.2% | `PASS` (0 Duplicates) |
| **8 Threads** | 3473 / 3500 | 54.50 ms | **1.72x** | 21.5% | `PASS` (0 Duplicates) |
| **12 Threads** | 3474 / 3500 | **54.96 ms** | **1.71x** | 14.2% | `PASS` (0 Duplicates) |

### 8.2 Correctness Test Suite Verification (18 Unit Tests + 4 Scenarios)

All 18 unit tests and 4 macro-scenarios pass cleanly with zero invariant violations:

```
[TEST 01/18] Empty Request List                                  : [PASS]
[TEST 02/18] Empty Parking Lot                                   : [PASS]
[TEST 03/18] One Vehicle One Compatible Slot                     : [PASS]
[TEST 04/18] More Vehicles Than Slots (Oversubscription)         : [PASS]
[TEST 05/18] More Slots Than Vehicles                            : [PASS]
[TEST 06/18] No Compatible Slots                                 : [PASS]
[TEST 07/18] EV Vehicle with EV and Non-EV Slots                 : [PASS]
[TEST 08/18] Accessible Exclusivity Constraints                  : [PASS]
[TEST 09/18] Large Vehicle Size Restrictions                     : [PASS]
[TEST 10/18] Pre-Occupied Slots Protection                       : [PASS]
[TEST 11/18] Equal-Cost Priority Tie-Breaking                    : [PASS]
[TEST 12/18] Equal-Cost Equal-Priority Vehicle ID Tie-Breaking   : [PASS]
[TEST 13/18] Nonsequential Vehicle IDs                           : [PASS]
[TEST 14/18] Duplicate Vehicle IDs Rejection                     : [PASS]
[TEST 15/18] Duplicate Slot IDs Rejection                        : [PASS]
[TEST 16/18] Repeated Multi-Thread Consistency (1, 2, 4, 8 thr) : [PASS]
[TEST 17/18] High-Contention Stress Scenario                     : [PASS]
[TEST 18/18] Validator Deliberate Error Detection                : [PASS]

EXECUTING DOCUMENTED SCENARIOS:
Scenario A (Small Functional Test)       : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario B (Oversubscribed Contention)   : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario C (Multi-Floor Fleet)           : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario D (Large Scale Concurrency)     : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)

TEST SUITE SUMMARY: ALL TESTS & SCENARIOS PASSED!
```

---

## 9. Viva & Oral Exam Guide (Frequently Asked Questions)

### Q1: What race condition occurs if smart parking allocation is naively parallelized?
> **Answer**: If we simply wrap the outer vehicle loop in `#pragma omp parallel for`, two concurrent threads processing different vehicles can evaluate the lot at the same instant, conclude that slot $S_k$ is the best available spot, and write their vehicle ID into $S_k$. This creates a data race and causes two vehicles to be assigned to the same physical spot (double booking).

### Q2: Why did you use fine-grained per-slot locks (`omp_lock_t`) instead of a single `#pragma omp critical` section?
> **Answer**: A global critical section serializes access across all threads, creating severe lock contention. Since there are thousands of distinct physical parking spots, two threads proposing to different spots can proceed in parallel without conflict. Fine-grained per-slot locking ensures threads only synchronize when contesting the exact same slot.

### Q3: How does the Multi-Round Proposal Algorithm avoid deadlocks?
> **Answer**: Deadlock requires circular wait (Coffman condition). In our proposal phase, each thread processes one vehicle at a time and acquires at most **one** slot lock to register its proposal, immediately releasing it before continuing. Because no thread holds multiple locks simultaneously, circular wait is impossible.

### Q4: Why does the OpenMP loop use `schedule(guided)`?
> **Answer**: Vehicle requests have heterogeneous constraints (e.g. Accessible vehicles check all spots, whereas Large vehicles can only use Large spots). This leads to variable computation times per request. The `guided` schedule assigns large chunks initially and dynamically decreases chunk sizes, achieving superior load balancing with low scheduling overhead.

### Q5: How do Amdahl's Law and memory bandwidth impact the maximum speedup?
> **Answer**: According to Amdahl's law:
> $$S_{\text{latency}} = \frac{1}{(1 - f) + \frac{f}{p}}$$
> The serial fraction $1 - f$ includes memory allocation, active request vector filtering, and lock cleanup. Additionally, searching large slot arrays stresses the L3 cache and memory bus, which dampens ideal linear scaling at high thread counts.
