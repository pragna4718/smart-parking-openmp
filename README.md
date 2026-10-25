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
    └── main.cpp                ### 5.1 Prerequisites
* **C++ Compiler**: `g++` with C++17 support and `-fopenmp` (e.g. MinGW-w64).
* **Make Utility**: `mingw32-make` (Windows) or `make` (Linux/macOS).
* **Python**: Python 3 with `matplotlib` for generating performance dashboards.

### 5.2 Build Commands (Windows & MinGW)
```bash
# Clean previous build artifacts and binaries safely
mingw32-make clean

# Compile the complete project with C++17, -O3 optimizations, and OpenMP
mingw32-make all

# Verify OpenMP environment and maximum hardware thread detection
mingw32-make hello
```

### 5.3 Command-Line Interface & Syntax
The executable supports both the original roadmap positional interface (equivalent to `./parking N M seed`) and named command-line flags:

#### Positional Syntax:
```bash
./smart_parking.exe [N] [M] [seed] [mode]
```
* **`N`**: Number of vehicle requests to generate (e.g., `3500`).
* **`M`**: Total parking slots (e.g., `5000`). Automatically factors into realistic floors, rows, and columns ($M = F \times R \times C$).
* **`seed`**: RNG seed for reproducible parking lot and vehicle stream generation (e.g., `42`).
* **`mode`**: Execution mode: `serial`, `parallel`, `both`, or `test` (default: `both`).

*Examples*:
```bash
# Run serial allocator baseline with 200 vehicles, 100 slots, seed 42
./smart_parking.exe 200 100 42 serial

# Run parallel allocator with 200 vehicles, 100 slots, seed 42
./smart_parking.exe 200 100 42 parallel

# Run both allocators for side-by-side comparison
./smart_parking.exe 500 250 100 both
```

#### Named Flag Options:
```text
  -N, --vehicles <N> Number of vehicle requests (default: 3500)
  -M, --slots <M>    Number of parking slots (default: 5000)
  -s, --seed <N>     Random seed for reproducible generation (default: 42)
  -t, --threads <N>  Number of OpenMP threads (default: OMP_NUM_THREADS / hardware max)
  -m, --mode <str>   Execution mode: 'serial', 'parallel', 'both', 'test' (default: both)
  --floors <N>       Number of parking floors (overrides automatic slot factoring)
  --rows <N>         Number of rows per floor
  --cols <N>         Number of columns per floor
  --benchmark        Run multi-thread scaling benchmark across thread counts
  --csv <path>       Save benchmark metrics to CSV file
  --test             Execute comprehensive 20 unit tests and 4 macro scenarios
  --help, -h         Show help screen
```

### 5.4 Configuring OpenMP Thread Count
The thread count can be configured via program argument or environment variable:
1. **Command-Line Option**: Pass `-t <threads>` or `--threads <threads>`. The program invokes `omp_set_num_threads(threads)` before parallel execution.
   ```powershell
   ./smart_parking.exe -N 4000 -M 5000 -t 8 --mode parallel
   ```
2. **Environment Variable**: Set `OMP_NUM_THREADS` in the shell environment. When no `-t` argument is supplied, OpenMP automatically honors `OMP_NUM_THREADS` (or defaults to `omp_get_max_threads()`).
   ```powershell
   $env:OMP_NUM_THREADS = "4"
   ./smart_parking.exe --benchmark
   ```

### 5.5 Running Correctness Tests
```bash
# Execute comprehensive 20 unit tests + 4 scenarios test suite
mingw32-make test
# or directly:
./smart_parking.exe --test
```

### 5.6 Running Benchmarks & Plotting
```bash
# Execute multi-threaded OpenMP scaling benchmark and record CSV
mingw32-make benchmark
# or customize workload scale:
./smart_parking.exe --floors 4 --rows 25 --cols 50 --vehicles 4000 --benchmark --csv benchmark_results.csv

# Generate high-resolution 3-panel performance graph (Time, Speedup, Efficiency)
python scripts/plot_results.py
```

---

## 6. Comprehensive Bug Audit & Targeted Engineering Fixes

A thorough audit of the original codebase revealed 11 critical bugs across memory safety, indexing, concurrency, compatibility, and capacity preservation:

| Bug Category | Location | Root Cause | Engineering Fix Applied |
| :--- | :--- | :--- | :--- |
| **1. Vehicle Identity vs Vector Indexing** | `src/parallel_alloc.cpp` | In the resolution phase, `results[win_vid]` used vehicle ID `req.id` as a direct array index. If vehicle IDs were nonsequential (e.g. ID `9999` with 20 requests), this triggered out-of-bounds writes and segfaults. | Added `req_index` to `SlotBid` struct, completely decoupling vehicle identity from vector position. `results[win_idx]` is now indexed safely by request index in $[0, N-1]$. |
| **2. Accessibility Compatibility Hole** | `src/cost.cpp` | `isCompatible(VehicleType::ACCESSIBLE, ...)` unconditionally returned `true`, illegally allowing Accessible vehicles into `COMPACT` and `EV` stalls in violation of Section 2.3 requirements. | Constrained Accessible vehicles strictly to `ACCESSIBLE`, `REGULAR`, and `LARGE` slots. |
| **3. Artificial Cost Clamping** | `src/cost.cpp` | `computeSlotCost` clamped negative total costs to `0.0`. This erased the relative benefit between close and distant EV stalls when the $-10.0$ incentive bonus was applied. | Removed artificial clamping so costs accurately reflect the continuous objective function. |
| **4. Arbiter Tie-Breaking Inversion** | `src/parallel_alloc.cpp` | Floating-point epsilon check `fabs(min_cost - slot_bids.cost) < 1e-6` was evaluated without checking if `min_cost > slot_bids.cost`, allowing higher-cost bids to displace lower-cost bids if priority was higher. | Replaced with strict epsilon ordering: strictly lower cost wins (`min_cost < cost - 1e-9`); only on numerical equivalence ($|\Delta| \le 1e-9$) is priority and then vehicle ID evaluated. |
| **5. Unassigned Request Pruning** | `src/parallel_alloc.cpp` | Vehicles that found zero compatible free slots in round $k$ remained in `active_req_indices` across all subsequent rounds, wasting CPU searching exhausted slot pools. | Tracked proposal viability per round (`vehicle_found_slot`). Vehicles with no candidate slots are pruned immediately since available slots strictly shrink each round. |
| **6. OpenMP Lock Lifecycle (RAII)** | `src/parallel_alloc.cpp` | Raw `omp_lock_t` array had manual destroy calls that were bypassed on early returns or exceptions, leaking OS synchronization handles. | Created `OpenMPLockArray` RAII wrapper guaranteeing automatic destruction across all exit paths. |
| **7. Input Sanity & Duplicate IDs** | `src/serial_alloc.cpp`, `src/parallel_alloc.cpp` | Duplicate vehicle or slot IDs were accepted silently, creating conflicting physical reservations. | Added explicit input validation throwing `std::invalid_argument` upon detecting duplicate vehicle or slot IDs. |
| **8. Permissive Invariant Validator** | `src/validator.cpp` | Original validator only checked duplicate slot counts; it missed missing results, duplicate results, unknown IDs, pre-occupied slots, non-finite costs, invalid success/failure combinations, and cost mismatches. | Strengthened `validateAllocation` to independently verify all 12 invariant categories and produce detailed error diagnostics. |
| **9. Monotonic Timing via OpenMP** | `src/main.cpp` | Allocation timing previously included clock conversion overhead or lacked OpenMP timer standardization. | Switched strictly to `omp_get_wtime()`, isolating exclusively the allocator execution from generation, validation, and console output. |
| **10. Large Slot Capacity Preservation** | `src/cost.h`, `src/cost.cpp` | Regular vehicles indifferent between a `REGULAR` slot and a `LARGE` slot at equal cost would take the `LARGE` slot arbitrarily, leaving large vehicles unplaceable. | Implemented `isPreferredCandidateSlot` tie-breaking that favors matching vehicle/slot types over flexible large bays. |
| **11. Accessible Stall Preservation** | `src/cost.h`, `src/cost.cpp` | Accessible vehicles indifferent between `ACCESSIBLE` and `REGULAR` slots would take `REGULAR` slots, depleting stalls for regular cars unable to use accessible spots. | Updated `isPreferredCandidateSlot` to prioritize dedicated `ACCESSIBLE` stalls for permitted vehicles. |

---

## 7. Deterministic Hand-Checkable Test Cases

The test suite includes 20 deterministic, manually verifiable test cases verifying edge cases, invariants, and tie-breaking:

| Test Case | Workload Input $(N, M)$ | Conditions & Constraints | Expected Allocation Outcome | Validation Status |
| :--- | :--- | :--- | :--- | :--- |
| **01. Empty Request List** | $N=0, M=4$ | Empty vehicle batch against a 4-slot lot. | 0 assigned, 0 unassigned; empty results vector. | `PASS` |
| **02. Empty Parking Lot** | $N=5, M=0$ | 5 vehicle requests against a lot with 0 slots. | 0 assigned, 5 unassigned; all vehicles fail gracefully. | `PASS` |
| **03. One Vehicle One Slot** | $N=1, M=1$ | 1 Regular car at (0,0,0) and 1 Regular slot at (0,0,0). | Assigned to Slot 0; cost = 4.0; 0 unassigned. | `PASS` |
| **04. Oversubscription ($N > M$)** | $N=5, M=2$ | 5 Regular cars competing for 2 Regular slots. | Exactly 2 vehicles assigned; 3 unassigned; 0 duplicates. | `PASS` |
| **05. Undersubscription ($N < M$)** | $N=3, M=10$ | 3 Regular cars in a 10-slot lot. | All 3 vehicles successfully assigned; 7 slots remain free. | `PASS` |
| **06. No Compatible Slots** | $N=1, M=2$ | 1 Large vehicle; 2 Compact slots. | 0 assigned, 1 unassigned (infinite cost/incompatible). | `PASS` |
| **07. EV Incentive vs Penalty** | $N=1, M=2$ | 1 EV car; Slot 0 is EV (dist 5); Slot 1 is Regular (dist 0). | Assigned to EV Slot 0 ($-10.0$ bonus vs $+15.0$ penalty). | `PASS` |
| **08. Accessible Exclusivity** | $N=2, M=1$ | 1 Regular, 1 Accessible; 1 Accessible slot. | Accessible car wins slot; Regular car turned away. | `PASS` |
| **09. Large Vehicle Restriction** | $N=1, M=3$ | 1 Large car; 1 Compact, 1 Regular, 1 Large slot. | Assigned strictly to Large slot (other slots forbidden). | `PASS` |
| **10. Pre-Occupied Slot Protection**| $N=1, M=2$ | 1 Regular car; Slot 0 is occupied; Slot 1 is free. | Assigned strictly to free Slot 1. | `PASS` |
| **11. Equal-Cost Priority Tie-Break**| $N=2, M=1$ | Identical cost: Car 1 (priority 1) vs Car 2 (priority 10). | Car 2 (priority 10) awarded slot; Car 1 rejected. | `PASS` |
| **12. Equal-Cost Equal-Priority ID** | $N=2, M=1$ | Identical cost & priority: Car ID 5 vs Car ID 42. | Car ID 5 (lower vehicle ID) awarded slot; Car 42 rejected. | `PASS` |
| **13. Nonsequential Vehicle IDs** | $N=3, M=3$ | Vehicle IDs `9999`, `42`, `1000`. | All assigned safely without array out-of-bounds indexing. | `PASS` |
| **14. Duplicate Vehicle IDs** | $N=2, M=2$ | Duplicate vehicle ID `10` provided in requests. | Allocator rejects input, throwing `std::invalid_argument`. | `PASS` |
| **15. Duplicate Slot IDs** | $N=2, M=2$ | Duplicate slot ID `0` provided in parking lot. | Allocator rejects input, throwing `std::invalid_argument`. | `PASS` |
| **16. Multi-Thread Consistency** | $N=20, M=50$ | Repeated runs over 1, 2, 4, 8 threads. | 100% deterministic results; identical counts & 0 duplicates. | `PASS` |
| **17. High-Contention Stress** | $N=50, M=2$ | 50 vehicles competing for 2 slots with 8 threads. | Exactly 2 vehicles parked; 48 turned away; 0 duplicates. | `PASS` |
| **18. Validator Error Detection** | Varied | 11 synthetic corrupted states (duplicates, bad IDs, costs). | Validator catches and flags all 11 corruptions. | `PASS` |
| **19. Large Bay Preservation** | $N=2, M=2$ | 1 Regular, 1 Large car; 1 Regular, 1 Large slot. | Regular car prefers Regular slot; Large car gets Large slot. | `PASS` |
| **20. Accessible Bay Preservation** | $N=2, M=2$ | 1 Accessible, 1 Regular; 1 Accessible, 1 Regular slot. | Accessible car prefers Accessible slot; Regular car gets slot. | `PASS` |

---

## 8. OpenMP Technical Note: Directives Inventory & Purpose

Every OpenMP directive and API call actually present in the codebase is documented below:

| Directive / API Call | Source Location | Operational Purpose | Concurrency Classification |
| :--- | :--- | :--- | :--- |
| `#pragma omp parallel for num_threads(threads_to_use) schedule(guided)` | `src/parallel_alloc.cpp:118` | Parallelizes the proposal search over active unassigned vehicles. Because heterogeneous vehicles evaluate differing candidate counts, `guided` dynamically balances load across threads. | **Independent Parallel Loop** |
| `omp_set_lock(slot_locks[best_slot_idx])`<br>`omp_unset_lock(...)` | `src/parallel_alloc.cpp:148, 174` | Provides fine-grained per-slot mutual exclusion. Synchronizes concurrent proposals competing for the exact same physical slot, enforcing deterministic arbitration without global serialization. | **Fine-Grained Mutex (Deadlock-Free)** |
| `#pragma omp parallel for num_threads(threads_to_use) schedule(static) reduction(+:new_assignments)` | `src/parallel_alloc.cpp:180` | Parallelizes the slot assignment commit phase over all $M$ slots. Accumulates newly assigned vehicle count safely using OpenMP tree reduction. | **Independent Write Loop with Reduction** |
| `omp_init_lock`<br>`omp_destroy_lock` | `src/parallel_alloc.cpp:20, 26` | Initializes and destroys the array of per-slot locks. Wrapped inside `OpenMPLockArray` RAII struct to prevent OS handle leaks. | **Synchronization Lifecycle Management** |
| `omp_get_wtime()` | `src/main.cpp:591, 605, 792, 821, 896, 908` | Measures wall-clock execution time with high precision ($< 1\,\mu\text{s}$ resolution). Isolates allocation runtime exclusively from input generation and printing. | **Monotonic Performance Profiling** |
| `omp_set_num_threads(threads)` | `src/main.cpp:763` | Sets the OpenMP thread pool size dynamically according to CLI arguments (`-t` or `--threads`). | **Runtime Environment Configuration** |
| `omp_get_max_threads()` | `src/main.cpp:765, 778, 803` | Queries the maximum execution threads configured by hardware or the `OMP_NUM_THREADS` environment variable. | **Hardware Concurrency Query** |

> [!NOTE]
> **Random Number Generation**: In this project, synthetic parking lot and vehicle stream generation (`src/generator.cpp`) executes sequentially in the harness prior to entering parallel regions. Random generation does **not** occur inside any parallel OpenMP regions; therefore, no per-thread RNG locking or thread-local generator state is required.

---

## 9. Allocation Quality Analysis: Serial Greedy vs. Parallel Proposal Arbiter

### Why Do Serial and Parallel Allocations Yield Slightly Different Parked Counts?

During scenario comparisons on constrained lots, the serial baseline and the parallel proposal engine exhibit small differences in total vehicles parked (e.g., Scenario B: 73 serial vs 71 parallel; Scenario C: 771 serial vs 756 parallel; Scenario D: 3934 serial vs 3855 parallel).

Crucially, **the parallel allocator achieves a substantially lower total solution cost** (e.g., Scenario B: 457.60 parallel vs 877.60 serial, a **$48\%$ cost reduction**; Scenario D: 28,099 parallel vs 34,080 serial, a **$17.5\%$ cost reduction**).

This difference stems from the fundamental algorithmic design:

1. **Serial Greedy (Arrival-Order Path Dependency)**:
   * Vehicles are assigned sequentially in request order ($i = 0, 1, \dots, N-1$).
   * Early vehicles grab whichever free slot minimizes their personal cost, regardless of whether subsequent vehicles desperately need that slot.
   * Path dependency can inadvertently leave large bays open if early regular vehicles happened to pick smaller bays.
   * Total system cost is high because early vehicles optimize locally without global competition.

2. **Parallel Proposal Arbiter (Simultaneous Competitive Auction)**:
   * In each round, **all** unassigned vehicles concurrently bid on their lowest-cost compatible slot.
   * Contested slots are arbitrated strictly by **lowest cost**, with ties broken by **higher priority** and **lower vehicle ID**.
   * Winning vehicles claim slots based on global merit and proximity rather than arbitrary arrival sequence.
   * In dense lots, flexible vehicles (`REGULAR`, `COMPACT`, `ACCESSIBLE`) whose destinations are close to a `LARGE` bay may find that bay cheaper than a distant regular slot. If they win that bay, the pool of `LARGE` stalls is consumed, preventing highly constrained `LARGE` trucks (which can park *only* in large bays) from finding a slot in subsequent rounds.

**Conclusion**: Both algorithms strictly satisfy all mutual exclusion, type compatibility, and concurrency invariants with **zero double-bookings**. The parallel proposal engine achieves vastly superior economic efficiency and pedestrian walking convenience at a minor trade-off in capacity under heavy saturation.

---

## 10. Experimental Results & Performance Analysis

### 10.1 Multi-Thread Scaling Benchmark ($M = 5,000$ slots, $N = 3,500$ vehicles, Seed 42)
*Environment: Windows 11, MinGW GCC 6.3.0 (32-bit), AMD/Intel Multi-Core Processor (12 Logical Threads).*
*Timing measured exclusively via `omp_get_wtime()`, excluding data generation and console I/O.*

| Threads ($p$) | Parked Count | Parallel Time (ms) | Speedup $S(p)$ | Efficiency $E(p)$ | Correctness Status |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **Serial Baseline** | 3500 / 3500 | 139.00 ms | 1.00x | 100.0% | `PASS` (0 Duplicates) |
| **1 Thread** | 3476 / 3500 | 289.00 ms | 0.48x | 48.1% | `PASS` (0 Duplicates) |
| **2 Threads** | 3476 / 3500 | 158.00 ms | 0.88x | 44.0% | `PASS` (0 Duplicates) |
| **4 Threads** | 3476 / 3500 | 101.00 ms | **1.38x** | 34.4% | `PASS` (0 Duplicates) |
| **8 Threads** | 3476 / 3500 | 68.00 ms | **2.04x** | 25.6% | `PASS` (0 Duplicates) |
| **12 Threads** | 3476 / 3500 | **69.00 ms** | **2.01x** | 16.8% | `PASS` (0 Duplicates) |

### 10.2 Correctness Test Suite Verification (20 Unit Tests + 4 Scenarios)
```
[TEST 01/20] Empty Request List                                  : [PASS]
[TEST 02/20] Empty Parking Lot                                   : [PASS]
[TEST 03/20] One Vehicle One Compatible Slot                     : [PASS]
[TEST 04/20] More Vehicles Than Slots (Oversubscription)         : [PASS]
[TEST 05/20] More Slots Than Vehicles                            : [PASS]
[TEST 06/20] No Compatible Slots                                 : [PASS]
[TEST 07/20] EV Vehicle with EV and Non-EV Slots                 : [PASS]
[TEST 08/20] Accessible Exclusivity Constraints                  : [PASS]
[TEST 09/20] Large Vehicle Size Restrictions                     : [PASS]
[TEST 10/20] Pre-Occupied Slots Protection                       : [PASS]
[TEST 11/20] Equal-Cost Priority Tie-Breaking                    : [PASS]
[TEST 12/20] Equal-Cost Equal-Priority Vehicle ID Tie-Breaking   : [PASS]
[TEST 13/20] Nonsequential Vehicle IDs                           : [PASS]
[TEST 14/20] Duplicate Vehicle IDs Rejection                     : [PASS]
[TEST 15/20] Duplicate Slot IDs Rejection                        : [PASS]
[TEST 16/20] Repeated Multi-Thread Consistency (1, 2, 4, 8 threads): [PASS]
[TEST 17/20] High-Contention Stress Scenario                     : [PASS]
[TEST 18/20] Validator Deliberate Error Detection                : [PASS]
[TEST 19/20] Candidate Slot Tie-Breaking Preserves Large Slot    : [PASS]
[TEST 20/20] Candidate Slot Tie-Breaking Preserves Accessible Slot: [PASS]

EXECUTING DOCUMENTED SCENARIOS:
Scenario A (Small Functional Test)       : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario B (Oversubscribed Contention)   : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario C (Multi-Floor Fleet)           : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)
Scenario D (Large Scale Concurrency)     : [PASS] ALL INVARIANTS SATISFIED (0 Duplicates)

TEST SUITE SUMMARY: ALL TESTS & SCENARIOS PASSED!
```

---

## 11. Viva & Oral Exam Guide (Frequently Asked Questions)

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
