#include "dqn_replacement.h"
#include <iostream>

DQNReplacement::DQNReplacement() : dqn_agent(nullptr), associated_cache(nullptr) {
    // Initialize DQN agent (state size and action size may depend on your feature space)
    dqn_agent = new DQNAgent(NUM_WAY, 5);  // For simplicity, assume state has 5 features
}

DQNReplacement::~DQNReplacement() {
    delete dqn_agent;
}

void DQNReplacement::initialize_replacement(CACHE* cache) {
    associated_cache = cache; // Link cache to replacement policy
    last_used_cycles[cache] = std::vector<uint64_t>(NUM_SET * NUM_WAY, 0); // Initialize last used cycles
}

uint32_t DQNReplacement::find_victim(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Build the state representation (this could include last access time, access type, etc.)
    std::vector<double> state = build_state(set, current_set, ip, full_addr, type);

    // Ask the DQN agent for the victim (selected way)
    int victim_way = dqn_agent->act(state);

    std::cout << "DQN selected victim at way: " << victim_way << std::endl;
    return victim_way;
}

void DQNReplacement::update_replacement_state(uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint32_t type, uint8_t hit) {
    // Update last used cycle if it's a miss or non-write access
    if (!hit || access_type{type} != access_type::WRITE) {
        last_used_cycles[associated_cache].at(set * NUM_WAY + way) = current_cycle;
    }

    // Calculate the reward based on hit/miss
    double reward = hit ? 1.0 : -1.0;

    // Update DQN agent with experience
    std::vector<double> next_state = build_state(set, current_set, ip, full_addr, type);
    dqn_agent->remember(last_state, way, reward, next_state);

    // Replay to learn from experiences
    dqn_agent->replay();
}

void DQNReplacement::replacement_final_stats() {
    // Output statistics after simulation ends (such as cache miss rate)
    std::cout << "Final statistics for DQN replacement:" << std::endl;
    // Here you can print the results of simulation, like miss rates, rewards, etc.
}

std::vector<double> DQNReplacement::build_state(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Create a state vector based on the current cache set and access details
    std::vector<double> state = {
        static_cast<double>(set),                      // Cache set index
        static_cast<double>(current_set->get_last_access_time()),  // Last access time
        static_cast<double>(ip % NUM_WAY),             // Example: Some derived feature from the IP
        static_cast<double>(full_addr),                // Full address
        static_cast<double>(type)                      // Access type (read/write)
    };

    return state;
}
