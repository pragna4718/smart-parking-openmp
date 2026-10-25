#include "generator.h"
#include "cost.h"
#include "validator.h"
#include <omp.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cerrno>
#if defined(_WIN32)
#include <windows.h>
#endif

struct BenchmarkEntry {
    int threads;
    int slots;
    int vehicles;
    double serial_time_ms;
    double parallel_time_ms;
    double speedup;
    double efficiency;
    bool is_valid;
};

// Helper functions for deterministic unit testing
bool testEmptyRequestList() {
    auto slots = generateParkingLot(1, 2, 2, 42);
    std::vector<VehicleRequest> requests;
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 4, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid && s_res.empty() && p_res.empty();
}

bool testEmptyParkingLot() {
    std::vector<ParkingSlot> slots;
    auto requests = generateRequests(5, 1, 5, 5, 42);
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 4, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 0 && p_val.assigned_count == 0 &&
           s_val.unassigned_count == 5 && p_val.unassigned_count == 5;
}

bool testOneVehicleOneSlot() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {10, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 1 && p_val.assigned_count == 1 &&
           s_res[0].slot_id == 0 && p_res[0].slot_id == 0;
}

bool testMoreVehiclesThanSlots() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {1, 0, 1, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {10, VehicleType::REGULAR, 0, 0, 0, 100.0, 1},
        {20, VehicleType::REGULAR, 0, 0, 0, 100.0, 2},
        {30, VehicleType::REGULAR, 0, 0, 0, 100.0, 3},
        {40, VehicleType::REGULAR, 0, 0, 0, 100.0, 4},
        {50, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 4, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_val.duplicate_count == 0 && p_val.duplicate_count == 0 &&
           s_val.assigned_count == 2 && p_val.assigned_count == 2 &&
           s_val.unassigned_count == 3 && p_val.unassigned_count == 3;
}

bool testMoreSlotsThanVehicles() {
    std::vector<ParkingSlot> slots;
    for (int i = 0; i < 10; ++i) {
        slots.push_back({i, 0, i, 0, SlotType::REGULAR, 2.0, false});
    }
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 1},
        {2, VehicleType::REGULAR, 0, 5, 0, 100.0, 1},
        {3, VehicleType::REGULAR, 0, 9, 0, 100.0, 1}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 4, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 3 && p_val.assigned_count == 3;
}

bool testNoCompatibleSlots() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::COMPACT, 2.0, false},
        {1, 0, 1, 0, SlotType::COMPACT, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::LARGE, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 0 && p_val.assigned_count == 0 &&
           s_val.unassigned_count == 1 && p_val.unassigned_count == 1;
}

bool testEVVehicleWithEVAndNonEVSlots() {
    // Slot 0 is EV at (0, 5, 0). Slot 1 is REGULAR at (0, 0, 0).
    // EV vehicle at (0, 0, 0).
    // Regular slot cost: dist=0, base_rate=2.0, non-EV penalty=+15.0 => cost = 0 + 4 + 15 = 19.0.
    // EV slot cost: dist=5, base_rate=2.0, EV incentive=-10.0 => cost = 5 + 4 - 10 = -1.0.
    // Cost of EV slot (-1.0) is much lower than REGULAR (19.0). Both must prefer EV slot.
    std::vector<ParkingSlot> slots = {
        {0, 0, 5, 0, SlotType::EV, 2.0, false},
        {1, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::EV, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_res[0].slot_id == 0 && p_res[0].slot_id == 0;
}

bool testAccessibleExclusivity() {
    // 1 ACCESSIBLE slot only.
    // Non-accessible vehicle must NOT be allocated to ACCESSIBLE slot.
    // Accessible vehicle CAN be allocated to ACCESSIBLE slot.
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::ACCESSIBLE, 2.0, false}
    };
    std::vector<VehicleRequest> req_regular = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto s_slots1 = slots;
    auto p_slots1 = slots;
    auto s_res1 = allocateSerial(s_slots1, req_regular, CostWeights());
    auto p_res1 = allocateParallel(p_slots1, req_regular, 2, CostWeights());
    bool reg_rejected = (s_res1[0].slot_id == -1 && p_res1[0].slot_id == -1);

    std::vector<VehicleRequest> req_acc = {
        {2, VehicleType::ACCESSIBLE, 0, 0, 0, 100.0, 5}
    };
    auto s_slots2 = slots;
    auto p_slots2 = slots;
    auto s_res2 = allocateSerial(s_slots2, req_acc, CostWeights());
    auto p_res2 = allocateParallel(p_slots2, req_acc, 2, CostWeights());
    bool acc_accepted = (s_res2[0].slot_id == 0 && p_res2[0].slot_id == 0);

    return reg_rejected && acc_accepted;
}

bool testLargeVehicleRestrictions() {
    // 1 COMPACT, 1 REGULAR, 1 EV, 1 LARGE slot.
    // 1 LARGE vehicle. Must ONLY be placed in LARGE slot.
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::COMPACT, 2.0, false},
        {1, 0, 1, 0, SlotType::REGULAR, 2.0, false},
        {2, 0, 2, 0, SlotType::EV, 2.0, false},
        {3, 0, 3, 0, SlotType::LARGE, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::LARGE, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    return s_res[0].slot_id == 3 && p_res[0].slot_id == 3;
}

bool testSlotsAlreadyOccupied() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, true}, // already occupied!
        {1, 0, 10, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_res[0].slot_id == 1 && p_res[0].slot_id == 1;
}

bool testEqualCostPriorityTieBreaking() {
    // 1 slot. Two vehicles with equal cost.
    // Vehicle 1 priority 3; Vehicle 2 priority 9.
    // Parallel arbiter MUST award slot to higher priority Vehicle 2!
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 3},
        {2, VehicleType::REGULAR, 0, 0, 0, 100.0, 9}
    };
    auto p_slots = slots;
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto p_val = validateAllocation(p_res, slots, requests);
    return p_val.is_valid && p_res[1].slot_id == 0 && p_res[0].slot_id == -1;
}

bool testEqualCostEqualPriorityVehicleIdTieBreaking() {
    // 1 slot. Two vehicles with equal cost AND equal priority 5.
    // Vehicle 1 ID 50; Vehicle 2 ID 20.
    // Parallel arbiter MUST award slot to lower vehicle ID (Vehicle 20, index 1)!
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {50, VehicleType::REGULAR, 0, 0, 0, 100.0, 5},
        {20, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto p_slots = slots;
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto p_val = validateAllocation(p_res, slots, requests);
    return p_val.is_valid && p_res[1].slot_id == 0 && p_res[0].slot_id == -1;
}

bool testNonsequentialVehicleIds() {
    std::vector<ParkingSlot> slots = {
        {101, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {202, 0, 1, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {9999, VehicleType::REGULAR, 0, 0, 0, 100.0, 5},
        {505, VehicleType::REGULAR, 0, 1, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);
    return s_val.is_valid && p_val.is_valid &&
           s_res.size() == 2 && p_res.size() == 2 &&
           s_res[0].vehicle_id == 9999 && p_res[0].vehicle_id == 9999 &&
           s_res[1].vehicle_id == 505 && p_res[1].vehicle_id == 505;
}

bool testDuplicateVehicleIdsRejection() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5},
        {1, VehicleType::REGULAR, 0, 1, 0, 100.0, 5} // duplicate ID 1
    };
    bool serial_threw = false;
    try {
        auto s_slots = slots;
        allocateSerial(s_slots, requests, CostWeights());
    } catch (const std::invalid_argument&) {
        serial_threw = true;
    }

    bool parallel_threw = false;
    try {
        auto p_slots = slots;
        allocateParallel(p_slots, requests, 2, CostWeights());
    } catch (const std::invalid_argument&) {
        parallel_threw = true;
    }

    // Also verify validator flags duplicate vehicle IDs in input requests
    std::vector<AllocationResult> dummy_results = {
        {1, 0, 2.0, true},
        {1, -1, COST_INFINITY, false}
    };
    auto report = validateAllocation(dummy_results, slots, requests);
    bool validator_caught = !report.is_valid;

    return serial_threw && parallel_threw && validator_caught;
}

bool testDuplicateSlotIdsRejection() {
    std::vector<ParkingSlot> slots = {
        {10, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {10, 0, 1, 0, SlotType::REGULAR, 2.0, false} // duplicate slot ID 10
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    bool serial_threw = false;
    try {
        auto s_slots = slots;
        allocateSerial(s_slots, requests, CostWeights());
    } catch (const std::invalid_argument&) {
        serial_threw = true;
    }

    bool parallel_threw = false;
    try {
        auto p_slots = slots;
        allocateParallel(p_slots, requests, 2, CostWeights());
    } catch (const std::invalid_argument&) {
        parallel_threw = true;
    }

    std::vector<AllocationResult> dummy_results = {
        {1, 10, 2.0, true}
    };
    auto report = validateAllocation(dummy_results, slots, requests);
    bool validator_caught = !report.is_valid;

    return serial_threw && parallel_threw && validator_caught;
}

bool testRepeatedThreadCounts() {
    auto original_slots = generateParkingLot(2, 5, 5, 42);
    auto requests = generateRequests(20, 2, 5, 5, 1337);

    for (int t : {1, 2, 4, 8}) {
        auto p_slots = original_slots;
        auto p_res = allocateParallel(p_slots, requests, t, CostWeights());
        auto p_val = validateAllocation(p_res, original_slots, requests);
        if (!p_val.is_valid || p_val.duplicate_count > 0 || p_val.incompatible_count > 0) {
            return false;
        }
    }
    return true;
}

bool testHighContention() {
    // 50 vehicles competing for 2 slots with 8 threads
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {1, 0, 1, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests;
    for (int i = 0; i < 50; ++i) {
        requests.push_back({i, VehicleType::REGULAR, 0, 0, 0, 100.0, i % 10 + 1});
    }
    auto p_slots = slots;
    auto p_res = allocateParallel(p_slots, requests, 8, CostWeights());
    auto p_val = validateAllocation(p_res, slots, requests);
    return p_val.is_valid && p_val.duplicate_count == 0 &&
           p_val.assigned_count == 2 && p_val.unassigned_count == 48;
}

bool testValidatorDeliberateErrors() {
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {1, 0, 1, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5},
        {2, VehicleType::REGULAR, 0, 1, 0, 100.0, 5}
    };

    // Subtest 1: Duplicate slot assignment (two cars get same slot)
    {
        std::vector<AllocationResult> bad_res = {
            {1, 0, 4.0, true},
            {2, 0, 4.0, true}
        };
        auto rep = validateAllocation(bad_res, slots, requests);
        if (rep.is_valid || rep.duplicate_count != 1) return false;
    }

    // Subtest 2: Non-existent slot ID
    {
        std::vector<AllocationResult> bad_res = {
            {1, 999, 4.0, true},
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 3: Unknown vehicle ID
    {
        std::vector<AllocationResult> bad_res = {
            {999, 0, 4.0, true},
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 4: Incompatible assignment (Regular vehicle in Accessible slot)
    {
        std::vector<ParkingSlot> acc_slots = {
            {0, 0, 0, 0, SlotType::ACCESSIBLE, 2.0, false}
        };
        std::vector<VehicleRequest> reg_req = {
            {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
        };
        std::vector<AllocationResult> bad_res = {
            {1, 0, 4.0, true}
        };
        auto rep = validateAllocation(bad_res, acc_slots, reg_req);
        if (rep.is_valid || rep.incompatible_count < 1) return false;
    }

    // Subtest 5: Invalid combinations: success=true with slot_id=-1
    {
        std::vector<AllocationResult> bad_res = {
            {1, -1, 4.0, true},
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 6: Invalid combinations: success=false with slot_id=0
    {
        std::vector<AllocationResult> bad_res = {
            {1, 0, COST_INFINITY, false},
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 7: Invalid combinations: success=false with finite cost
    {
        std::vector<AllocationResult> bad_res = {
            {1, -1, 10.0, false},
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 8: Assigned to slot already occupied in original input
    {
        std::vector<ParkingSlot> occ_slots = {
            {0, 0, 0, 0, SlotType::REGULAR, 2.0, true}
        };
        std::vector<VehicleRequest> single_req = {
            {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
        };
        std::vector<AllocationResult> bad_res = {
            {1, 0, 4.0, true}
        };
        if (validateAllocation(bad_res, occ_slots, single_req).is_valid) return false;
    }

    // Subtest 9: Inconsistent cost reported
    {
        std::vector<AllocationResult> bad_res = {
            {1, 0, 999.99, true}, // expected is 4.0
            {2, 1, 4.0, true}
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 10: Missing result record
    {
        std::vector<AllocationResult> bad_res = {
            {1, 0, 4.0, true} // vehicle 2 omitted
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    // Subtest 11: Duplicate result record
    {
        std::vector<AllocationResult> bad_res = {
            {1, 0, 4.0, true},
            {1, 1, 4.0, true} // vehicle 1 duplicate
        };
        if (validateAllocation(bad_res, slots, requests).is_valid) return false;
    }

    return true;
}

bool testCandidateSlotTieBreakingPreservesLargeSlot() {
    // Slot 0 is LARGE, Slot 1 is REGULAR.
    // Both are at (0, 0, 0) with identical base rate 2.0.
    // Vehicle 1 is REGULAR at (0, 0, 0).
    // Vehicle 2 is LARGE at (0, 0, 0).
    // Both vehicles have equal priority (5).
    // With capacity-preserving tie-breaking:
    // Vehicle 1 has equal cost for Slot 0 (LARGE) and Slot 1 (REGULAR).
    // It must prefer its matching type (Slot 1, REGULAR), leaving Slot 0 (LARGE)
    // for Vehicle 2.
    // Both vehicles must be successfully parked in both Serial and Parallel.
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::LARGE, 2.0, false},
        {1, 0, 0, 0, SlotType::REGULAR, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::REGULAR, 0, 0, 0, 100.0, 5},
        {2, VehicleType::LARGE, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);

    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 2 && p_val.assigned_count == 2 &&
           s_res[0].slot_id == 1 && s_res[1].slot_id == 0 &&
           p_res[0].slot_id == 1 && p_res[1].slot_id == 0;
}

bool testCandidateSlotTieBreakingPreservesAccessibleSlot() {
    // Slot 0 is REGULAR, Slot 1 is ACCESSIBLE.
    // Both are at (0, 0, 0) with identical base rate 2.0.
    // Vehicle 1 is ACCESSIBLE at (0, 0, 0).
    // Vehicle 2 is REGULAR at (0, 0, 0).
    // Both vehicles have equal priority (5).
    // With capacity-preserving tie-breaking:
    // Vehicle 1 has equal cost for Slot 0 (REGULAR) and Slot 1 (ACCESSIBLE).
    // It must prefer the dedicated ACCESSIBLE slot (Slot 1), leaving Slot 0 (REGULAR)
    // for Vehicle 2 (which cannot use ACCESSIBLE slots).
    // Both vehicles must be successfully parked in both Serial and Parallel.
    std::vector<ParkingSlot> slots = {
        {0, 0, 0, 0, SlotType::REGULAR, 2.0, false},
        {1, 0, 0, 0, SlotType::ACCESSIBLE, 2.0, false}
    };
    std::vector<VehicleRequest> requests = {
        {1, VehicleType::ACCESSIBLE, 0, 0, 0, 100.0, 5},
        {2, VehicleType::REGULAR, 0, 0, 0, 100.0, 5}
    };
    auto s_slots = slots;
    auto p_slots = slots;
    auto s_res = allocateSerial(s_slots, requests, CostWeights());
    auto p_res = allocateParallel(p_slots, requests, 2, CostWeights());
    auto s_val = validateAllocation(s_res, slots, requests);
    auto p_val = validateAllocation(p_res, slots, requests);

    return s_val.is_valid && p_val.is_valid &&
           s_val.assigned_count == 2 && p_val.assigned_count == 2 &&
           s_res[0].slot_id == 1 && s_res[1].slot_id == 0 &&
           p_res[0].slot_id == 1 && p_res[1].slot_id == 0;
}

bool runScenario(const std::string& name, int floors, int rows, int cols, int vehicles, int threads) {
    std::cout << "\n======================================================\n";
    std::cout << " RUNNING SCENARIO: " << name << "\n";
    std::cout << " Configuration: " << floors << " floors, "
              << (floors * rows * cols) << " total slots, "
              << vehicles << " vehicle requests, "
              << threads << " OpenMP threads\n";
    std::cout << "======================================================\n";

    auto original_slots = generateParkingLot(floors, rows, cols, 100);
    auto requests = generateRequests(vehicles, floors, cols, rows, 200);

    // Dynamic repetitions for short workloads to ensure reliable monotonic timing
    int repeat_count = 1;
    if (vehicles <= 50) {
        repeat_count = 200;
    } else if (vehicles <= 250) {
        repeat_count = 50;
    }

    // 1. Serial Allocation
    std::vector<AllocationResult> serial_results;
    auto serial_slots = original_slots;
    double t0 = omp_get_wtime();
    for (int rep = 0; rep < repeat_count; ++rep) {
        serial_slots = original_slots;
        serial_results = allocateSerial(serial_slots, requests, CostWeights());
    }
    double t1 = omp_get_wtime();
    double serial_ms = ((t1 - t0) * 1000.0) / repeat_count;

    auto serial_report = validateAllocation(serial_results, original_slots, requests);
    printValidationReport("Serial Baseline (" + name + ")", serial_report);

    // 2. Parallel Allocation
    std::vector<AllocationResult> parallel_results;
    auto parallel_slots = original_slots;
    double t2 = omp_get_wtime();
    for (int rep = 0; rep < repeat_count; ++rep) {
        parallel_slots = original_slots;
        parallel_results = allocateParallel(parallel_slots, requests, threads, CostWeights());
    }
    double t3 = omp_get_wtime();
    double parallel_ms = ((t3 - t2) * 1000.0) / repeat_count;

    auto parallel_report = validateAllocation(parallel_results, original_slots, requests);
    printValidationReport("OpenMP Parallel (" + name + ")", parallel_report);

    // 3. Comparison Metrics
    double speedup = (parallel_ms > 1e-6) ? (serial_ms / parallel_ms) : 0.0;
    double efficiency = (threads > 0) ? (speedup / threads) : 0.0;
    double serial_throughput = (serial_ms > 1e-6) ? (vehicles / (serial_ms / 1000.0)) : 0.0;
    double parallel_throughput = (parallel_ms > 1e-6) ? (vehicles / (parallel_ms / 1000.0)) : 0.0;

    std::cout << "--- Performance Comparison (" << name << ") ---\n";
    std::cout << " Serial Parked / Total:   " << serial_report.assigned_count << " / " << vehicles << "\n";
    std::cout << " Parallel Parked / Total: " << parallel_report.assigned_count << " / " << vehicles << "\n";
    std::cout << " Serial Execution Time:   " << std::fixed << std::setprecision(3) << serial_ms << " ms";
    if (repeat_count > 1) std::cout << " (avg over " << repeat_count << " runs)";
    std::cout << "\n";
    std::cout << " Parallel Execution Time: " << std::fixed << std::setprecision(3) << parallel_ms << " ms";
    if (repeat_count > 1) std::cout << " (avg over " << repeat_count << " runs)";
    std::cout << "\n";
    std::cout << " Speedup:                 " << std::fixed << std::setprecision(2) << speedup << "x\n";
    std::cout << " Parallel Efficiency:     " << std::fixed << std::setprecision(1) << (efficiency * 100.0) << "%\n";
    std::cout << " Serial Throughput:       " << std::fixed << std::setprecision(0) << serial_throughput << " req/sec\n";
    std::cout << " Parallel Throughput:     " << std::fixed << std::setprecision(0) << parallel_throughput << " req/sec\n";
    std::cout << " Serial Correctness:      " << (serial_report.is_valid ? "[PASS]" : "[FAILED]") << "\n";
    std::cout << " Parallel Correctness:    " << (parallel_report.is_valid ? "[SUCCESS - ZERO DUPLICATES]" : "[FAILED]") << "\n";
    std::cout << "======================================================\n\n";

    return serial_report.is_valid && parallel_report.is_valid;
}

int runTestSuite() {
    std::cout << "\n######################################################\n";
    std::cout << " EXECUTING COMPREHENSIVE CORRECTNESS TEST SUITE\n";
    std::cout << "######################################################\n\n";

    struct TestCase {
        std::string name;
        bool (*func)();
    };

    std::vector<TestCase> unit_tests = {
        {"Empty Request List", testEmptyRequestList},
        {"Empty Parking Lot", testEmptyParkingLot},
        {"One Vehicle One Compatible Slot", testOneVehicleOneSlot},
        {"More Vehicles Than Slots (Oversubscription)", testMoreVehiclesThanSlots},
        {"More Slots Than Vehicles", testMoreSlotsThanVehicles},
        {"No Compatible Slots", testNoCompatibleSlots},
        {"EV Vehicle with EV and Non-EV Slots", testEVVehicleWithEVAndNonEVSlots},
        {"Accessible Exclusivity Constraints", testAccessibleExclusivity},
        {"Large Vehicle Size Restrictions", testLargeVehicleRestrictions},
        {"Pre-Occupied Slots Protection", testSlotsAlreadyOccupied},
        {"Equal-Cost Priority Tie-Breaking", testEqualCostPriorityTieBreaking},
        {"Equal-Cost Equal-Priority Vehicle ID Tie-Breaking", testEqualCostEqualPriorityVehicleIdTieBreaking},
        {"Nonsequential Vehicle IDs", testNonsequentialVehicleIds},
        {"Duplicate Vehicle IDs Rejection", testDuplicateVehicleIdsRejection},
        {"Duplicate Slot IDs Rejection", testDuplicateSlotIdsRejection},
        {"Repeated Multi-Thread Consistency (1, 2, 4, 8 threads)", testRepeatedThreadCounts},
        {"High-Contention Stress Scenario", testHighContention},
        {"Validator Deliberate Error Detection", testValidatorDeliberateErrors},
        {"Candidate Slot Tie-Breaking Preserves Large Slot", testCandidateSlotTieBreakingPreservesLargeSlot},
        {"Candidate Slot Tie-Breaking Preserves Accessible Slot", testCandidateSlotTieBreakingPreservesAccessibleSlot}
    };

    bool all_passed = true;
    int test_num = 1;
    for (const auto& tc : unit_tests) {
        std::cout << "[TEST " << (test_num < 10 ? "0" : "") << test_num
                  << "/" << unit_tests.size() << "] "
                  << std::setw(52) << std::left << tc.name << ": " << std::flush;
        test_num++;
        bool pass = false;
        try {
            pass = tc.func();
        } catch (const std::exception& e) {
            std::cout << "[EXCEPTION] " << e.what() << "\n";
            pass = false;
        }
        if (pass) {
            std::cout << "[PASS]\n";
        } else {
            std::cout << "[FAIL]\n";
            all_passed = false;
        }
    }

    std::cout << "\n------------------------------------------------------\n";
    std::cout << " EXECUTING DOCUMENTED SCENARIOS (A, B, C, D)\n";
    std::cout << "------------------------------------------------------\n";

    bool scA = runScenario("Scenario A: Small Functional Test", 2, 5, 5, 20, 4);
    bool scB = runScenario("Scenario B: Oversubscribed Contention", 2, 5, 10, 200, 4);
    bool scC = runScenario("Scenario C: Multi-Floor Fleet", 4, 10, 25, 800, 4);
    bool scD = runScenario("Scenario D: Large Scale Concurrency", 4, 25, 50, 4000, 8);

    bool scenarios_passed = scA && scB && scC && scD;

    std::cout << "######################################################\n";
    if (all_passed && scenarios_passed) {
        std::cout << " TEST SUITE SUMMARY: ALL TESTS & SCENARIOS PASSED!\n";
        std::cout << "######################################################\n\n";
        return 0;
    } else {
        std::cout << " TEST SUITE SUMMARY: FAILURES DETECTED!\n";
        std::cout << "######################################################\n\n";
        return 1;
    }
}

void factorSlots(int total_slots, int& floors, int& rows, int& cols) {
    if (total_slots <= 0) total_slots = 5000;
    if (total_slots < 50) {
        floors = 1;
        rows = 1;
        cols = total_slots;
    } else if (total_slots <= 200) {
        floors = 2;
        int per_floor = (total_slots + floors - 1) / floors;
        rows = (per_floor <= 25) ? 5 : 10;
        cols = (per_floor + rows - 1) / rows;
    } else if (total_slots <= 1200) {
        floors = 4;
        int per_floor = (total_slots + floors - 1) / floors;
        rows = 10;
        cols = (per_floor + rows - 1) / rows;
    } else {
        floors = 4;
        int per_floor = (total_slots + floors - 1) / floors;
        rows = 25;
        cols = (per_floor + rows - 1) / rows;
    }
}

int main(int argc, char* argv[]) {
    // Default parameters
    int floors = 4;
    int rows = 25;
    int cols = 50; // 4 * 25 * 50 = 5000 slots
    int total_slots = 5000;
    int vehicles = 3500;
    unsigned int seed = 42;
    int threads = 0; // 0 means default to omp_get_max_threads() or OMP_NUM_THREADS
    std::string mode = "both";
    std::string csv_file = "";
    bool run_benchmark = false;
    bool explicit_floors = false;
    bool explicit_slots = false;

    // Support positional arguments: ./smart_parking.exe [N] [M] [seed] [mode]
    int pos_idx = 1;
    if (pos_idx < argc && argv[pos_idx][0] != '-') {
        vehicles = std::atoi(argv[pos_idx++]);
    }
    if (pos_idx < argc && argv[pos_idx][0] != '-') {
        total_slots = std::atoi(argv[pos_idx++]);
        explicit_slots = true;
    }
    if (pos_idx < argc && argv[pos_idx][0] != '-') {
        seed = static_cast<unsigned int>(std::strtoul(argv[pos_idx++], nullptr, 10));
    }
    if (pos_idx < argc && argv[pos_idx][0] != '-') {
        mode = argv[pos_idx++];
    }

    // Parse CLI flag options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--floors" && i + 1 < argc) {
            floors = std::atoi(argv[++i]);
            explicit_floors = true;
        } else if (arg == "--rows" && i + 1 < argc) {
            rows = std::atoi(argv[++i]);
            explicit_floors = true;
        } else if (arg == "--cols" && i + 1 < argc) {
            cols = std::atoi(argv[++i]);
            explicit_floors = true;
        } else if ((arg == "--vehicles" || arg == "-N") && i + 1 < argc) {
            vehicles = std::atoi(argv[++i]);
        } else if ((arg == "--slots" || arg == "-M") && i + 1 < argc) {
            total_slots = std::atoi(argv[++i]);
            explicit_slots = true;
        } else if ((arg == "--seed" || arg == "-s") && i + 1 < argc) {
            seed = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 10));
        } else if ((arg == "--threads" || arg == "-t") && i + 1 < argc) {
            threads = std::atoi(argv[++i]);
        } else if ((arg == "--mode" || arg == "-m") && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            csv_file = argv[++i];
        } else if (arg == "--benchmark") {
            run_benchmark = true;
        } else if (arg == "--test") {
            mode = "test";
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Parallel Smart Parking Allocation (OpenMP)\n"
                      << "Usage: " << argv[0] << " [N] [M] [seed] [mode]\n"
                      << "   or: " << argv[0] << " [options]\n\n"
                      << "Positional Arguments (equivalent to ./parking N M seed):\n"
                      << "  N                  Vehicle count (default: 3500)\n"
                      << "  M                  Parking slot count (default: 5000)\n"
                      << "  seed               RNG seed for lot and request generation (default: 42)\n"
                      << "  mode               Execution mode: 'serial', 'parallel', 'both', 'test' (default: both)\n\n"
                      << "Options:\n"
                      << "  -N, --vehicles <N> Number of vehicle requests (default: 3500)\n"
                      << "  -M, --slots <M>    Number of parking slots (default: 5000)\n"
                      << "  -s, --seed <N>     Random seed for reproducible generation (default: 42)\n"
                      << "  -t, --threads <N>  Number of OpenMP threads (default: OMP_NUM_THREADS / hardware max)\n"
                      << "  -m, --mode <str>   Execution mode: 'serial', 'parallel', 'both', 'test' (default: both)\n"
                      << "  --floors <N>       Number of parking floors (overrides automatic slot factoring)\n"
                      << "  --rows <N>         Number of rows per floor\n"
                      << "  --cols <N>         Number of columns per floor\n"
                      << "  --benchmark        Run multi-thread scaling benchmark\n"
                      << "  --csv <path>       Save benchmark metrics to CSV file\n"
                      << "  --help, -h         Show this help screen\n";
            return 0;
        }
    }

    if (threads > 0) {
        omp_set_num_threads(threads);
    } else {
        threads = omp_get_max_threads();
    }

    if (mode == "test") {
        return runTestSuite();
    }

    if (explicit_slots && !explicit_floors) {
        factorSlots(total_slots, floors, rows, cols);
    } else {
        total_slots = floors * rows * cols;
    }

    if (run_benchmark) {
        std::cout << "\n======================================================\n";
        std::cout << " OPENMP SCALING BENCHMARK SUITE\n";
        std::cout << " Problem Size: " << total_slots << " slots, " << vehicles << " vehicles\n";
        std::cout << " Available Cores/Threads: " << omp_get_max_threads() << "\n";
        std::cout << " RNG Seed: " << seed << "\n";
        std::cout << "======================================================\n\n";

        auto original_slots = generateParkingLot(floors, rows, cols, seed);
        if (static_cast<int>(original_slots.size()) > total_slots) {
            original_slots.resize(total_slots);
        }
        auto requests = generateRequests(vehicles, floors, cols, rows, seed + 1000);

        // 1. Measure Serial Baseline
        std::cout << "Running serial baseline... " << std::flush;
        auto s_slots = original_slots;
        double s_t0 = omp_get_wtime();
        auto s_res = allocateSerial(s_slots, requests, CostWeights());
        double s_t1 = omp_get_wtime();
        double serial_ms = (s_t1 - s_t0) * 1000.0;
        auto s_val = validateAllocation(s_res, original_slots, requests);
        std::cout << "Done: " << std::fixed << std::setprecision(2) << serial_ms << " ms ("
                  << s_val.assigned_count << "/" << vehicles << " parked, "
                  << (s_val.is_valid ? "Valid" : "INVALID") << ")\n\n";

        // 2. Thread count sweep
        std::vector<int> thread_counts = {1, 2, 4, 8};
        int max_t = omp_get_max_threads();
        if (std::find(thread_counts.begin(), thread_counts.end(), max_t) == thread_counts.end()) {
            thread_counts.push_back(max_t);
        }
        std::sort(thread_counts.begin(), thread_counts.end());

        std::vector<BenchmarkEntry> entries;

        std::cout << std::setw(8) << "Threads"
                  << std::setw(12) << "Parked"
                  << std::setw(15) << "Parallel (ms)"
                  << std::setw(12) << "Speedup"
                  << std::setw(15) << "Efficiency"
                  << std::setw(12) << "Valid?" << "\n";
        std::cout << std::string(74, '-') << "\n";

        for (int t : thread_counts) {
            auto p_slots = original_slots;
            double p_t0 = omp_get_wtime();
            auto p_res = allocateParallel(p_slots, requests, t, CostWeights());
            double p_t1 = omp_get_wtime();
            double p_ms = (p_t1 - p_t0) * 1000.0;

            auto p_val = validateAllocation(p_res, original_slots, requests);
            double spd = (p_ms > 1e-6) ? (serial_ms / p_ms) : 0.0;
            double eff = (t > 0) ? (spd / t) : 0.0;

            std::cout << std::setw(8) << t
                      << std::setw(8) << p_val.assigned_count << "/" << vehicles
                      << std::setw(13) << std::fixed << std::setprecision(2) << p_ms
                      << std::setw(11) << std::fixed << std::setprecision(2) << spd << "x"
                      << std::setw(14) << std::fixed << std::setprecision(1) << (eff * 100.0) << "%"
                      << std::setw(12) << (p_val.is_valid ? "YES [PASS]" : "FAIL") << "\n";

            BenchmarkEntry entry;
            entry.threads = t;
            entry.slots = total_slots;
            entry.vehicles = vehicles;
            entry.serial_time_ms = serial_ms;
            entry.parallel_time_ms = p_ms;
            entry.speedup = spd;
            entry.efficiency = eff;
            entry.is_valid = p_val.is_valid;
            entries.push_back(entry);
        }

        // Write CSV if requested
        if (!csv_file.empty()) {
            std::ofstream ofs(csv_file);
            if (!ofs.is_open()) {
                #if defined(_WIN32)
                std::string cmd = "powershell -NoProfile -Command \"Remove-Item -Force '" + csv_file + "' -ErrorAction SilentlyContinue\"";
                std::system(cmd.c_str());
                #endif
                ofs.clear();
                ofs.open(csv_file);
            }

            if (ofs.is_open()) {
                ofs << "threads,slots,vehicles,serial_time_ms,parallel_time_ms,speedup,efficiency,is_valid\n";
                for (const auto& e : entries) {
                    ofs << e.threads << "," << e.slots << "," << e.vehicles << ","
                        << std::fixed << std::setprecision(4)
                        << e.serial_time_ms << "," << e.parallel_time_ms << ","
                        << e.speedup << "," << e.efficiency << ","
                        << (e.is_valid ? 1 : 0) << "\n";
                }
                ofs.close();
                std::cout << "\n[INFO] Benchmark CSV successfully written to: " << csv_file << "\n";
            } else {
                #if defined(_WIN32)
                DWORD last_err = GetLastError();
                #else
                int last_err = 0;
                #endif
                std::cerr << "[ERROR] Could not open CSV file for writing: " << csv_file
                          << " (errno: " << errno << ", GetLastError: " << last_err << ")\n";
            }
        }

        return 0;
    }

    // Standard run
    std::cout << "\n======================================================\n";
    std::cout << " SMART PARKING ALLOCATION SIMULATION\n";
    std::cout << " Slots: " << total_slots << " | Vehicles: " << vehicles
              << " | Seed: " << seed
              << " | Threads: " << threads << " | Mode: " << mode << "\n";
    std::cout << "======================================================\n";

    auto original_slots = generateParkingLot(floors, rows, cols, seed);
    if (static_cast<int>(original_slots.size()) > total_slots) {
        original_slots.resize(total_slots);
    }
    auto requests = generateRequests(vehicles, floors, cols, rows, seed + 1000);

    if (mode == "serial" || mode == "both") {
        auto slots_copy = original_slots;
        double t0 = omp_get_wtime();
        auto results = allocateSerial(slots_copy, requests, CostWeights());
        double t1 = omp_get_wtime();
        double ms = (t1 - t0) * 1000.0;

        auto report = validateAllocation(results, original_slots, requests);
        printValidationReport("Serial Baseline", report);
        std::cout << " >> Serial Wall-Clock Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
    }

    if (mode == "parallel" || mode == "both") {
        auto slots_copy = original_slots;
        double t0 = omp_get_wtime();
        auto results = allocateParallel(slots_copy, requests, threads, CostWeights());
        double t1 = omp_get_wtime();
        double ms = (t1 - t0) * 1000.0;

        auto report = validateAllocation(results, original_slots, requests);
        printValidationReport("OpenMP Parallel (" + std::to_string(threads) + " threads)", report);
        std::cout << " >> Parallel Wall-Clock Time: " << std::fixed << std::setprecision(2) << ms << " ms\n";
    }

    return 0;
}
