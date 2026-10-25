#include "cost.h"
#include <cmath>
#include <cstdlib>

bool isCompatible(VehicleType v_type, SlotType s_type) {
    // Accessible slots are legally reserved exclusively for accessible permit holders
    if (s_type == SlotType::ACCESSIBLE && v_type != VehicleType::ACCESSIBLE) {
        return false;
    }

    switch (v_type) {
        case VehicleType::ACCESSIBLE:
            // Accessible vehicle prefers ACCESSIBLE, but can use REGULAR/LARGE if necessary
            return s_type == SlotType::ACCESSIBLE ||
                   s_type == SlotType::REGULAR ||
                   s_type == SlotType::LARGE;

        case VehicleType::COMPACT:
            // Compact vehicle fits in COMPACT, REGULAR, LARGE, or EV
            return s_type == SlotType::COMPACT ||
                   s_type == SlotType::REGULAR ||
                   s_type == SlotType::LARGE ||
                   s_type == SlotType::EV;

        case VehicleType::REGULAR:
            // Regular car cannot fit in compact, fits in REGULAR, LARGE, or EV
            return s_type == SlotType::REGULAR ||
                   s_type == SlotType::LARGE ||
                   s_type == SlotType::EV;

        case VehicleType::LARGE:
            // Large SUV / truck only fits in LARGE slots
            return s_type == SlotType::LARGE;

        case VehicleType::EV:
            // EV vehicle prefers EV, but can park in REGULAR or LARGE if needed
            return s_type == SlotType::EV ||
                   s_type == SlotType::REGULAR ||
                   s_type == SlotType::LARGE;

        default:
            return false;
    }
}

double computeSlotCost(const VehicleRequest& req, const ParkingSlot& slot, const CostWeights& weights) {
    if (slot.is_occupied) {
        return COST_INFINITY;
    }

    if (!isCompatible(req.type, slot.type)) {
        return COST_INFINITY;
    }

    // 1. Horizontal Manhattan distance
    double dist_walk = std::abs(slot.x - req.target_x) + std::abs(slot.y - req.target_y);

    // 2. Vertical floor penalty
    double dist_floor = std::abs(slot.floor - req.target_floor) * weights.floor_cost_multiplier;

    // 3. Slot suitability bonus / penalty
    double type_penalty = 0.0;
    if (req.type == VehicleType::EV && slot.type == SlotType::EV) {
        type_penalty -= 10.0; // Incentive for charging station
    } else if (req.type == VehicleType::EV && slot.type != SlotType::EV) {
        type_penalty += 15.0; // Penalty for EV not getting a charger
    } else if (req.type == VehicleType::COMPACT && slot.type == SlotType::LARGE) {
        type_penalty += 5.0;  // Suboptimal space usage
    }

    // 4. Weighted total cost
    double total_cost = (weights.w_dist * dist_walk) +
                        (weights.w_floor * dist_floor) +
                        (weights.w_price * slot.base_rate) +
                        type_penalty;

    return total_cost;
}

bool isPreferredCandidateSlot(const VehicleRequest& req, const ParkingSlot& cand, const ParkingSlot& current_best) {
    // 1. Direct type match: prefer slot type matching vehicle type
    bool cand_match = (cand.type == static_cast<SlotType>(req.type));
    bool curr_match = (current_best.type == static_cast<SlotType>(req.type));
    if (cand_match != curr_match) {
        return cand_match;
    }

    // 2. Preserve LARGE slots for LARGE vehicles
    if (req.type != VehicleType::LARGE) {
        bool cand_is_large = (cand.type == SlotType::LARGE);
        bool curr_is_large = (current_best.type == SlotType::LARGE);
        if (cand_is_large != curr_is_large) {
            return curr_is_large; // Prefer cand if current is LARGE and cand is NOT LARGE
        }
    }

    // 3. Preserve EV slots for EV vehicles
    if (req.type != VehicleType::EV) {
        bool cand_is_ev = (cand.type == SlotType::EV);
        bool curr_is_ev = (current_best.type == SlotType::EV);
        if (cand_is_ev != curr_is_ev) {
            return curr_is_ev; // Prefer cand if current is EV and cand is NOT EV
        }
    }

    // 4. Preserve dedicated ACCESSIBLE slots for ACCESSIBLE vehicles
    if (req.type == VehicleType::ACCESSIBLE) {
        bool cand_is_acc = (cand.type == SlotType::ACCESSIBLE);
        bool curr_is_acc = (current_best.type == SlotType::ACCESSIBLE);
        if (cand_is_acc != curr_is_acc) {
            return cand_is_acc;
        }
    }

    // 5. Deterministic tie-breaker: lower slot ID
    return cand.id < current_best.id;
}
