#ifndef COST_H
#define COST_H

#include "generator.h"
#include <limits>

constexpr double COST_INFINITY = 1e9;

struct CostWeights {
    double w_dist;    // Weight for horizontal walking distance (grid units)
    double w_floor;   // Weight for vertical floor difference
    double w_price;   // Weight for base parking fee
    double floor_cost_multiplier; // Equivalent grid distance penalty per floor difference

    CostWeights(double wd = 1.0, double wf = 15.0, double wp = 2.0, double fcm = 1.0)
        : w_dist(wd), w_floor(wf), w_price(wp), floor_cost_multiplier(fcm) {}
};

// Returns true if vehicle can legally park in the given slot type
bool isCompatible(VehicleType v_type, SlotType s_type);

// Computes the weighted objective cost of assigning slot to vehicle.
// Returns COST_INFINITY if the slot is incompatible or occupied.
double computeSlotCost(const VehicleRequest& req, const ParkingSlot& slot, const CostWeights& weights = CostWeights());

// Deterministic candidate slot tie-breaker when two slots offer identical cost.
// Prefers exact vehicle type match and preserves constrained slot types (LARGE, EV, ACCESSIBLE).
bool isPreferredCandidateSlot(const VehicleRequest& req, const ParkingSlot& cand, const ParkingSlot& current_best);

#endif // COST_H
