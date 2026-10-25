#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "generator.h"
#include "cost.h"
#include <vector>
#include <string>

struct ValidationReport {
    bool is_valid;
    int total_requests;
    int assigned_count;
    int unassigned_count;
    int duplicate_count;
    int incompatible_count;
    double total_cost;
    double avg_cost;
    std::string error_message;
};

// Validates correctness of allocation results against original lot and requests
ValidationReport validateAllocation(
    const std::vector<AllocationResult>& results,
    const std::vector<ParkingSlot>& original_slots,
    const std::vector<VehicleRequest>& requests,
    const CostWeights& weights = CostWeights()
);

// Prints a formatted summary of the validation report
void printValidationReport(const std::string& label, const ValidationReport& report);

#endif // VALIDATOR_H
