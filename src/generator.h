#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include <vector>
#include <cstdint>

enum class SlotType : uint8_t {
    COMPACT = 0,
    REGULAR = 1,
    LARGE = 2,
    EV = 3,
    ACCESSIBLE = 4
};

enum class VehicleType : uint8_t {
    COMPACT = 0,
    REGULAR = 1,
    LARGE = 2,
    EV = 3,
    ACCESSIBLE = 4
};

struct ParkingSlot {
    int id;              // Unique slot identifier [0, num_slots - 1]
    int floor;           // Floor level (0 = ground floor, 1, 2, ...)
    int x;               // Grid coordinate X
    int y;               // Grid coordinate Y
    SlotType type;       // Type of parking space
    double base_rate;    // Base cost per hour / flat base cost
    bool is_occupied;    // Allocation status
};

struct VehicleRequest {
    int id;              // Unique request / vehicle ID [0, num_requests - 1]
    VehicleType type;    // Vehicle classification
    int target_floor;    // Preferred floor (e.g., closest to office/mall entrance)
    int target_x;        // Preferred entry point or destination X coordinate
    int target_y;        // Preferred entry point or destination Y coordinate
    double max_dist;     // Maximum acceptable distance penalty
    int priority;        // Request priority (higher priority gets tie-breaker advantage)
};

struct AllocationResult {
    int vehicle_id;      // Vehicle ID
    int slot_id;         // Assigned slot ID (-1 if unassigned)
    double cost;         // Computed cost for this assignment (INF if unassigned)
    bool success;        // True if successfully allocated
};

// Forward declaration of CostWeights struct
struct CostWeights;

// Generates synthetic parking lot grid with realistic distribution of slot types
std::vector<ParkingSlot> generateParkingLot(int num_floors, int rows_per_floor, int cols_per_floor, unsigned int seed = 42);

// Generates synthetic stream of vehicle parking requests
std::vector<VehicleRequest> generateRequests(int num_vehicles, int num_floors, int max_x, int max_y, unsigned int seed = 1337);

// Utility functions for string representation
std::string slotTypeToString(SlotType type);
std::string vehicleTypeToString(VehicleType type);

// Serial greedy allocation baseline
std::vector<AllocationResult> allocateSerial(
    std::vector<ParkingSlot>& slots,
    const std::vector<VehicleRequest>& requests,
    const CostWeights& weights
);

// OpenMP parallel proposal-based allocation
std::vector<AllocationResult> allocateParallel(
    std::vector<ParkingSlot>& slots,
    const std::vector<VehicleRequest>& requests,
    int num_threads,
    const CostWeights& weights
);

#endif // GENERATOR_H
