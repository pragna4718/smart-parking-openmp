#include "generator.h"
#include "cost.h"
#include <omp.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <unordered_set>
#include <stdexcept>
#include <string>

namespace {

// RAII wrapper for an array of OpenMP locks
struct OpenMPLockArray {
    std::vector<omp_lock_t> locks;

    explicit OpenMPLockArray(size_t n) : locks(n) {
        for (size_t i = 0; i < n; ++i) {
            omp_init_lock(&locks[i]);
        }
    }

    ~OpenMPLockArray() {
        for (size_t i = 0; i < locks.size(); ++i) {
            omp_destroy_lock(&locks[i]);
        }
    }

    // Non-copyable
    OpenMPLockArray(const OpenMPLockArray&) = delete;
    OpenMPLockArray& operator=(const OpenMPLockArray&) = delete;

    omp_lock_t* operator[](size_t i) { return &locks[i]; }
};

struct SlotBid {
    int req_index;   // Request vector index [0, total_requests - 1]
    int vehicle_id;  // Vehicle ID (used for tie-breaking)
    double cost;     // Proposed assignment cost
    int priority;    // Vehicle priority (used for tie-breaking)
};

constexpr double COST_EPSILON = 1e-9;

} // anonymous namespace

std::vector<AllocationResult> allocateParallel(
    std::vector<ParkingSlot>& slots,
    const std::vector<VehicleRequest>& requests,
    int num_threads,
    const CostWeights& weights
) {
    // Validate uniqueness of input vehicle IDs
    std::unordered_set<int> seen_req_ids;
    seen_req_ids.reserve(requests.size());
    for (const auto& req : requests) {
        if (!seen_req_ids.insert(req.id).second) {
            throw std::invalid_argument("Duplicate vehicle ID detected in requests: " + std::to_string(req.id));
        }
    }

    // Validate uniqueness of input slot IDs
    std::unordered_set<int> seen_slot_ids;
    seen_slot_ids.reserve(slots.size());
    for (const auto& slot : slots) {
        if (!seen_slot_ids.insert(slot.id).second) {
            throw std::invalid_argument("Duplicate slot ID detected in slots: " + std::to_string(slot.id));
        }
    }

    const int total_requests = static_cast<int>(requests.size());
    const int total_slots = static_cast<int>(slots.size());

    // Allocate and initialize 1:1 result records for each input request
    std::vector<AllocationResult> results(total_requests);
    for (int i = 0; i < total_requests; ++i) {
        results[i].vehicle_id = requests[i].id;
        results[i].slot_id = -1;
        results[i].cost = COST_INFINITY;
        results[i].success = false;
    }

    if (total_requests == 0 || total_slots == 0) {
        return results;
    }

    int threads_to_use = (num_threads > 0) ? num_threads : omp_get_max_threads();
    if (threads_to_use < 1) {
        threads_to_use = 1;
    }

    // Initialize fine-grained OpenMP locks with RAII cleanup
    OpenMPLockArray slot_locks(total_slots);

    // Active unassigned request indices
    std::vector<int> active_req_indices(total_requests);
    for (int i = 0; i < total_requests; ++i) {
        active_req_indices[i] = i;
    }

    // Multi-Round Greedy Proposal Algorithm:
    // In each round:
    // 1. All active vehicles concurrently propose to their best available slot.
    // 2. An arbiter decides the winner for each contested slot using fine-grained synchronization.
    //    Deterministic tie-breaking: lower cost -> higher priority -> lower vehicle ID.
    // 3. Winning vehicles claim slots; losers remain active for subsequent rounds.
    // 4. Vehicles finding no compatible candidate slots in a round are pruned.
    // 5. Continues until no unassigned vehicles remain or no new assignments can be made.
    while (!active_req_indices.empty()) {
        // Track proposals for each slot in this round
        std::vector<SlotBid> slot_bids(total_slots, {-1, -1, COST_INFINITY, -1});
        std::vector<uint8_t> vehicle_found_slot(total_requests, 0);

        const int num_active = static_cast<int>(active_req_indices.size());

        // Step 1: Parallel Proposal Phase
        #pragma omp parallel for num_threads(threads_to_use) schedule(guided)
        for (int idx = 0; idx < num_active; ++idx) {
            int r_i = active_req_indices[idx];
            const auto& req = requests[r_i];

            int best_slot_idx = -1;
            double min_cost = COST_INFINITY;

            // Search across all available slots
            for (int s = 0; s < total_slots; ++s) {
                if (slots[s].is_occupied) {
                    continue;
                }

                double cost = computeSlotCost(req, slots[s], weights);
                if (cost < min_cost - COST_EPSILON) {
                    min_cost = cost;
                    best_slot_idx = s;
                } else if (std::fabs(cost - min_cost) <= COST_EPSILON) {
                    if (best_slot_idx == -1 || isPreferredCandidateSlot(req, slots[s], slots[best_slot_idx])) {
                        min_cost = cost;
                        best_slot_idx = s;
                    }
                }
            }

            // If a compatible slot was found, submit proposal under slot lock
            if (best_slot_idx != -1 && min_cost < COST_INFINITY) {
                vehicle_found_slot[r_i] = 1;

                omp_set_lock(slot_locks[best_slot_idx]);

                // Arbiter:
                // 1. Lower cost first
                // 2. On equal cost (within epsilon), higher priority
                // 3. On equal priority, lower vehicle ID
                bool is_better = false;
                if (slot_bids[best_slot_idx].req_index == -1) {
                    is_better = true;
                } else if (min_cost < slot_bids[best_slot_idx].cost - COST_EPSILON) {
                    is_better = true;
                } else if (std::fabs(min_cost - slot_bids[best_slot_idx].cost) <= COST_EPSILON) {
                    if (req.priority > slot_bids[best_slot_idx].priority) {
                        is_better = true;
                    } else if (req.priority == slot_bids[best_slot_idx].priority && req.id < slot_bids[best_slot_idx].vehicle_id) {
                        is_better = true;
                    }
                }

                if (is_better) {
                    slot_bids[best_slot_idx].req_index = r_i;
                    slot_bids[best_slot_idx].vehicle_id = req.id;
                    slot_bids[best_slot_idx].cost = min_cost;
                    slot_bids[best_slot_idx].priority = req.priority;
                }

                omp_unset_lock(slot_locks[best_slot_idx]);
            }
        }

        // Step 2: Parallel Resolution Phase
        int new_assignments = 0;
        #pragma omp parallel for num_threads(threads_to_use) schedule(static) reduction(+:new_assignments)
        for (int s = 0; s < total_slots; ++s) {
            int win_idx = slot_bids[s].req_index;
            if (win_idx != -1) {
                slots[s].is_occupied = true;
                results[win_idx].slot_id = slots[s].id;
                results[win_idx].cost = slot_bids[s].cost;
                results[win_idx].success = true;
                new_assignments++;
            }
        }

        // If no new assignments were possible, no remaining vehicle can be accommodated
        if (new_assignments == 0) {
            break;
        }

        // Step 3: Filter unassigned requests for the next round.
        // Keep requests that are not assigned AND found at least one candidate slot this round.
        std::vector<int> next_active;
        next_active.reserve(active_req_indices.size() - new_assignments);
        for (int r_i : active_req_indices) {
            if (!results[r_i].success && vehicle_found_slot[r_i]) {
                next_active.push_back(r_i);
            }
        }

        if (next_active.empty()) {
            break;
        }

        active_req_indices = std::move(next_active);
    }

    return results;
}
