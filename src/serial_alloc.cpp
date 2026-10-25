#include "generator.h"
#include "cost.h"
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <cmath>

namespace {
constexpr double COST_EPSILON = 1e-9;
}

std::vector<AllocationResult> allocateSerial(
    std::vector<ParkingSlot>& slots,
    const std::vector<VehicleRequest>& requests,
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

    std::vector<AllocationResult> results;
    results.reserve(requests.size());

    for (const auto& req : requests) {
        int best_slot_idx = -1;
        double min_cost = COST_INFINITY;

        // Exhaustive search over all slots for the best available slot
        for (size_t s_idx = 0; s_idx < slots.size(); ++s_idx) {
            if (slots[s_idx].is_occupied) {
                continue;
            }

            double cost = computeSlotCost(req, slots[s_idx], weights);
            if (cost < min_cost - COST_EPSILON) {
                min_cost = cost;
                best_slot_idx = static_cast<int>(s_idx);
            } else if (std::fabs(cost - min_cost) <= COST_EPSILON) {
                if (best_slot_idx == -1 || isPreferredCandidateSlot(req, slots[s_idx], slots[best_slot_idx])) {
                    min_cost = cost;
                    best_slot_idx = static_cast<int>(s_idx);
                }
            }
        }

        AllocationResult res;
        res.vehicle_id = req.id;

        if (best_slot_idx != -1 && min_cost < COST_INFINITY) {
            slots[best_slot_idx].is_occupied = true;
            res.slot_id = slots[best_slot_idx].id;
            res.cost = min_cost;
            res.success = true;
        } else {
            res.slot_id = -1;
            res.cost = COST_INFINITY;
            res.success = false;
        }

        results.push_back(res);
    }

    return results;
}
