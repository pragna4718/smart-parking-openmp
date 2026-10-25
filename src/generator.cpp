#include "generator.h"
#include <random>
#include <cmath>

std::string slotTypeToString(SlotType type) {
    switch (type) {
        case SlotType::COMPACT:    return "COMPACT";
        case SlotType::REGULAR:    return "REGULAR";
        case SlotType::LARGE:      return "LARGE";
        case SlotType::EV:         return "EV";
        case SlotType::ACCESSIBLE: return "ACCESSIBLE";
        default:                   return "UNKNOWN";
    }
}

std::string vehicleTypeToString(VehicleType type) {
    switch (type) {
        case VehicleType::COMPACT:    return "COMPACT";
        case VehicleType::REGULAR:    return "REGULAR";
        case VehicleType::LARGE:      return "LARGE";
        case VehicleType::EV:         return "EV";
        case VehicleType::ACCESSIBLE: return "ACCESSIBLE";
        default:                      return "UNKNOWN";
    }
}

std::vector<ParkingSlot> generateParkingLot(int num_floors, int rows_per_floor, int cols_per_floor, unsigned int seed) {
    int total_slots = num_floors * rows_per_floor * cols_per_floor;
    std::vector<ParkingSlot> slots;
    slots.reserve(total_slots);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist_rate(2.0, 5.0);
    std::uniform_real_distribution<double> type_prob(0.0, 1.0);

    int id_counter = 0;
    for (int f = 0; f < num_floors; ++f) {
        for (int r = 0; r < rows_per_floor; ++r) {
            for (int c = 0; c < cols_per_floor; ++c) {
                ParkingSlot slot;
                slot.id = id_counter++;
                slot.floor = f;
                slot.x = c;
                slot.y = r;
                slot.base_rate = std::round(dist_rate(rng) * 10.0) / 10.0;
                slot.is_occupied = false;

                // Slot type distribution
                // Ground floor (floor 0) has more accessible spots near entrance (c < 5)
                double p = type_prob(rng);
                if (f == 0 && (r < 2 || c < 2)) {
                    slot.type = SlotType::ACCESSIBLE;
                } else if (p < 0.10) {
                    slot.type = SlotType::COMPACT;
                } else if (p < 0.70) {
                    slot.type = SlotType::REGULAR;
                } else if (p < 0.85) {
                    slot.type = SlotType::LARGE;
                } else if (p < 0.95) {
                    slot.type = SlotType::EV;
                } else {
                    slot.type = SlotType::ACCESSIBLE;
                }

                slots.push_back(slot);
            }
        }
    }

    return slots;
}

std::vector<VehicleRequest> generateRequests(int num_vehicles, int num_floors, int max_x, int max_y, unsigned int seed) {
    std::vector<VehicleRequest> requests;
    requests.reserve(num_vehicles);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> floor_dist(0, std::max(0, num_floors - 1));
    std::uniform_int_distribution<int> x_dist(0, std::max(0, max_x - 1));
    std::uniform_int_distribution<int> y_dist(0, std::max(0, max_y - 1));
    std::uniform_real_distribution<double> type_prob(0.0, 1.0);
    std::uniform_int_distribution<int> priority_dist(1, 10);
    std::uniform_real_distribution<double> max_dist_dist(50.0, 500.0);

    for (int i = 0; i < num_vehicles; ++i) {
        VehicleRequest req;
        req.id = i;
        req.target_floor = floor_dist(rng);
        req.target_x = x_dist(rng);
        req.target_y = y_dist(rng);
        req.priority = priority_dist(rng);
        req.max_dist = max_dist_dist(rng);

        double p = type_prob(rng);
        if (p < 0.15) {
            req.type = VehicleType::COMPACT;
        } else if (p < 0.70) {
            req.type = VehicleType::REGULAR;
        } else if (p < 0.85) {
            req.type = VehicleType::LARGE;
        } else if (p < 0.95) {
            req.type = VehicleType::EV;
        } else {
            req.type = VehicleType::ACCESSIBLE;
        }

        requests.push_back(req);
    }

    return requests;
}
