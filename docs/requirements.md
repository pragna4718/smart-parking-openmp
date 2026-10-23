# Requirements & Test Scenarios Specification

## 1. Project Overview
The Parallel Smart Parking Allocation project implements a concurrent slot assignment engine in C++17 using OpenMP. It allocates heterogeneous vehicle requests to multi-floor parking lot slots according to a multi-objective cost function while preventing double-bookings.

---

## 2. Functional Requirements

### 2.1 Domain Model
1. **Parking Facility**:
   - Multi-floor grid structure ($F$ floors, $R$ rows, $C$ columns). Total slots $M = F \times R \times C$.
   - Slot attributes: `id`, `floor`, coordinates `(x, y)`, `type`, `base_rate`, `is_occupied`.
   - Slot Types: `COMPACT`, `REGULAR`, `LARGE`, `EV`, `ACCESSIBLE`.

2. **Vehicle Requests**:
   - Incoming batch of $N$ vehicle requests.
   - Request attributes: `id`, `type`, target destination `(target_floor, target_x, target_y)`, `priority`, `max_dist`.
   - Vehicle Types: `COMPACT`, `REGULAR`, `LARGE`, `EV`, `ACCESSIBLE`.

### 2.2 Objective Cost Function
$$\text{Cost}(V, S) = w_{\text{dist}} \cdot D_{\text{walk}}(V, S) + w_{\text{floor}} \cdot \Delta F(V, S) + w_{\text{price}} \cdot S.\text{base\_rate} + P_{\text{type}}(V, S)$$

- **Walking Distance**: Manhattan distance $|S.x - V.target\_x| + |S.y - V.target\_y|$.
- **Vertical Penalty**: Floor difference $|S.floor - V.target\_floor| \times \mu_{\text{floor}}$.
- **Base Fee**: Base cost rate of slot.
- **Incentives & Penalties**:
  - EV in EV slot: $-10.0$ bonus.
  - EV in non-EV slot: $+15.0$ penalty.
  - Compact in Large slot: $+5.0$ space inefficiency penalty.
  - Incompatible slot: $\infty$ cost.

### 2.3 Compatibility Rules
- `COMPACT` vehicle $\rightarrow$ COMPACT, REGULAR, LARGE, EV
- `REGULAR` vehicle $\rightarrow$ REGULAR, LARGE, EV
- `LARGE` vehicle $\rightarrow$ LARGE only
- `EV` vehicle $\rightarrow$ EV (preferred), REGULAR, LARGE
- `ACCESSIBLE` vehicle $\rightarrow$ ACCESSIBLE (reserved), REGULAR, LARGE
- Non-accessible vehicles CANNOT park in ACCESSIBLE slots.

---

## 3. Concurrency & Invariant Requirements

1. **Mutual Exclusion (Zero Double-Booking)**:
   - For any slot $S_j$, at most one vehicle $V_i$ may be assigned to $S_j$.
   - Must be verified with `duplicate_count == 0` on every parallel execution.

2. **Deterministic Tie-Breaking**:
   - Equal cost candidates are decided by:
     1. Higher priority vehicle.
     2. Lower vehicle ID.

3. **Deadlock Freedom**:
   - Threads acquire at most one lock per proposal, eliminating circular wait conditions.

---

## 4. Test Scenarios

### Scenario A: Small Functional Baseline
- **Configuration**: 2 floors, 5 rows, 5 columns (50 slots), 20 vehicle requests, 4 threads.
- **Objective**: Verify basic compatibility, cost evaluation, and lack of race conditions on small workloads.

### Scenario B: Oversubscription / Heavy Contention
- **Configuration**: 2 floors, 5 rows, 10 columns (100 slots), 200 vehicle requests, 4 threads.
- **Objective**: Verify behavior when requests exceed available capacity ($N > M$). Ensure excess vehicles are rejected gracefully without crash or double-booking.

### Scenario C: Multi-Floor Heterogeneous Fleet
- **Configuration**: 4 floors, 10 rows, 25 columns (1,000 slots), 800 vehicle requests, 4 threads.
- **Objective**: Test floor travel penalty and mixed vehicle fleets (EV chargers, accessibility permits).

### Scenario D: High Concurrency Scalability
- **Configuration**: 4 floors, 25 rows, 50 columns (5,000 slots), 4,000 vehicle requests, 8-12 threads.
- **Objective**: Stress-test multi-threaded proposal arbiter, measuring speedup and throughput.
