#include "validator.h"
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cmath>

ValidationReport validateAllocation(
    const std::vector<AllocationResult>& results,
    const std::vector<ParkingSlot>& original_slots,
    const std::vector<VehicleRequest>& requests,
    const CostWeights& weights
) {
    ValidationReport report;
    report.is_valid = true;
    report.total_requests = static_cast<int>(requests.size());
    report.assigned_count = 0;
    report.unassigned_count = 0;
    report.duplicate_count = 0;
    report.incompatible_count = 0;
    report.total_cost = 0.0;
    report.avg_cost = 0.0;
    report.error_message = "";

    std::ostringstream err_stream;

    // 1. Validate uniqueness of vehicle IDs in input requests batch
    std::unordered_map<int, const VehicleRequest*> req_map;
    req_map.reserve(requests.size());
    for (const auto& req : requests) {
        if (!req_map.emplace(req.id, &req).second) {
            report.is_valid = false;
            err_stream << "[ERROR] Duplicate vehicle ID " << req.id << " in input requests batch\n";
        }
    }

    // 2. Validate uniqueness of slot IDs in input parking lot
    std::unordered_map<int, const ParkingSlot*> slot_map;
    slot_map.reserve(original_slots.size());
    for (const auto& slot : original_slots) {
        if (!slot_map.emplace(slot.id, &slot).second) {
            report.is_valid = false;
            err_stream << "[ERROR] Duplicate slot ID " << slot.id << " in input parking lot\n";
        }
    }

    // 3. Verify total result count matches request count
    if (results.size() != requests.size()) {
        report.is_valid = false;
        err_stream << "[ERROR] Result count mismatch: " << results.size()
                   << " results for " << requests.size() << " requests\n";
    }

    // 4. Map and validate individual result records
    std::unordered_map<int, int> result_count_per_vehicle;
    std::unordered_map<int, int> slot_assignment_counts;

    for (const auto& res : results) {
        result_count_per_vehicle[res.vehicle_id]++;
        if (result_count_per_vehicle[res.vehicle_id] > 1) {
            report.is_valid = false;
            err_stream << "[ERROR] Duplicate result record for vehicle ID " << res.vehicle_id << "\n";
        }

        auto r_it = req_map.find(res.vehicle_id);
        if (r_it == req_map.end()) {
            report.is_valid = false;
            err_stream << "[ERROR] Unknown vehicle ID " << res.vehicle_id << " in allocation results\n";
            continue;
        }

        const VehicleRequest* req = r_it->second;

        // Check for NaN or invalid cost
        if (std::isnan(res.cost)) {
            report.is_valid = false;
            err_stream << "[ERROR] NaN cost detected for vehicle " << res.vehicle_id << "\n";
            continue;
        }

        if (!res.success) {
            // Unsuccessful assignment contract:
            // slot_id MUST be -1, cost MUST be COST_INFINITY
            report.unassigned_count++;
            if (res.slot_id != -1) {
                report.is_valid = false;
                err_stream << "[ERROR] Unsuccessful result for vehicle " << res.vehicle_id
                           << " has invalid slot_id " << res.slot_id << " (expected -1)\n";
            }
            if (res.cost < COST_INFINITY - 1e-4) {
                report.is_valid = false;
                err_stream << "[ERROR] Unsuccessful result for vehicle " << res.vehicle_id
                           << " has finite cost " << res.cost << " (expected >= COST_INFINITY)\n";
            }
        } else {
            // Successful assignment contract:
            // slot_id MUST be >= 0, cost MUST be finite (< COST_INFINITY)
            report.assigned_count++;
            report.total_cost += res.cost;

            if (res.slot_id < 0) {
                report.is_valid = false;
                err_stream << "[ERROR] Successful result for vehicle " << res.vehicle_id
                           << " has negative slot_id " << res.slot_id << "\n";
                continue;
            }

            if (res.cost >= COST_INFINITY || std::isinf(res.cost)) {
                report.is_valid = false;
                err_stream << "[ERROR] Successful result for vehicle " << res.vehicle_id
                           << " has infinite cost " << res.cost << "\n";
            }

            // Check duplicate physical slot assignments
            slot_assignment_counts[res.slot_id]++;
            if (slot_assignment_counts[res.slot_id] > 1) {
                report.duplicate_count++;
                report.is_valid = false;
                err_stream << "[CRITICAL RACE CONDITION] Slot " << res.slot_id
                           << " assigned " << slot_assignment_counts[res.slot_id]
                           << " times! (Duplicate for vehicle " << res.vehicle_id << ")\n";
            }

            // Check slot existence
            auto s_it = slot_map.find(res.slot_id);
            if (s_it == slot_map.end()) {
                report.is_valid = false;
                err_stream << "[ERROR] Vehicle " << res.vehicle_id
                           << " assigned to non-existent slot ID " << res.slot_id << "\n";
                continue;
            }

            const ParkingSlot* slot = s_it->second;

            // Check if slot was already occupied in original input
            if (slot->is_occupied) {
                report.is_valid = false;
                err_stream << "[ERROR] Vehicle " << res.vehicle_id
                           << " assigned to slot " << slot->id
                           << " which was ALREADY OCCUPIED in initial parking lot\n";
            }

            // Check vehicle/slot compatibility
            if (!isCompatible(req->type, slot->type)) {
                report.incompatible_count++;
                report.is_valid = false;
                err_stream << "[ERROR] Incompatible assignment: Vehicle " << req->id
                           << " (" << vehicleTypeToString(req->type)
                           << ") in Slot " << slot->id
                           << " (" << slotTypeToString(slot->type) << ")\n";
            }

            // Hard constraint: non-accessible vehicle must not occupy accessible slot
            if (slot->type == SlotType::ACCESSIBLE && req->type != VehicleType::ACCESSIBLE) {
                report.incompatible_count++;
                report.is_valid = false;
                err_stream << "[ERROR] Accessible slot " << slot->id
                           << " illegally assigned to non-accessible vehicle " << req->id << "\n";
            }

            // Cost consistency check with defined cost function
            double expected_cost = computeSlotCost(*req, *slot, weights);
            if (std::abs(res.cost - expected_cost) > 1e-4) {
                report.is_valid = false;
                err_stream << "[ERROR] Cost inconsistency for vehicle " << req->id
                           << " in slot " << slot->id
                           << ": reported " << res.cost << " != expected " << expected_cost << "\n";
            }
        }
    }

    // 5. Check for missing results for any input requests
    for (const auto& req : requests) {
        if (result_count_per_vehicle.find(req.id) == result_count_per_vehicle.end()) {
            report.is_valid = false;
            err_stream << "[ERROR] Missing result for input request vehicle ID " << req.id << "\n";
        }
    }

    // 6. Verify assigned and unassigned counts sum to total requests
    if (report.assigned_count + report.unassigned_count != report.total_requests) {
        report.is_valid = false;
        err_stream << "[ERROR] Invariant violated: assigned_count (" << report.assigned_count
                   << ") + unassigned_count (" << report.unassigned_count
                   << ") != total_requests (" << report.total_requests << ")\n";
    }

    if (report.assigned_count > 0) {
        report.avg_cost = report.total_cost / report.assigned_count;
    } else {
        report.avg_cost = 0.0;
    }

    report.error_message = err_stream.str();
    return report;
}

void printValidationReport(const std::string& label, const ValidationReport& report) {
    std::cout << "\n========================================\n";
    std::cout << " Validation Report: " << label << "\n";
    std::cout << "========================================\n";
    std::cout << " Status:              " << (report.is_valid ? "[PASS] ALL INVARIANTS SATISFIED" : "[FAIL] INVARIANTS VIOLATED") << "\n";
    std::cout << " Total Requests:      " << report.total_requests << "\n";
    std::cout << " Successfully Parked: " << report.assigned_count << "\n";
    std::cout << " Unassigned / Turned: " << report.unassigned_count << "\n";
    std::cout << " Duplicate Bookings:  " << report.duplicate_count << "\n";
    std::cout << " Incompatible Slots:  " << report.incompatible_count << "\n";
    std::cout << " Total Solution Cost: " << std::fixed << std::setprecision(2) << report.total_cost << "\n";
    std::cout << " Average Cost / Car:  " << std::fixed << std::setprecision(2) << report.avg_cost << "\n";
    if (!report.is_valid) {
        std::cout << " Details:\n" << report.error_message;
    }
    std::cout << "========================================\n\n";
}
