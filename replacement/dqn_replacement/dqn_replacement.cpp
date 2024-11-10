#include "dqn_replacement.h"
#include <iostream>
#include <algorithm>

// Use this to store the last access times for each cache
namespace
{
    std::map<CACHE*, std::vector<uint64_t>> last_used_cycles;
}

// Constructor to initialize the DQN agent and other members
DQNReplacement::DQNReplacement() : dqn_agent(nullptr) {
    // Initialize DQN agent (state size and action size may depend on your feature space)
    dqn_agent = new DQNAgent(NUM_WAY, 5);  // Assume state has 5 features, NUM_WAY is the action space
}

// Destructor to clean up the DQN agent
DQNReplacement::~DQNReplacement() {
    delete dqn_agent;  // Clean up the DQN agent
}

// Override the `initialize_replacement` method from CACHE
void CACHE::initialize_replacement() {
    // Initialize the replacement strategy for DQN
    ::last_used_cycles[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY, 0);  // Initialize last used cycles
    // DQN agent initialization can happen here if necessary (it was already done in the constructor of DQNReplacement)
}

// Override the `find_victim` method from CACHE
uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Build the state vector based on cache set and other features
    std::vector<double> state = build_state(set, current_set, ip, full_addr, type);

    // Use the DQN agent to decide which cache line to evict
    int victim_way = dqn_agent->act(state);  // The agent chooses a victim

    std::cout << "DQN selected victim at way: " << victim_way << std::endl;
    return victim_way;  // Return the victim cache line (way)
}

// Update the replacement state after an access
void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit) {
    // Mark the way as being used on the current cycle
    if (!hit || access_type{type} != access_type::WRITE) {  // Skip this for writeback hits
        ::last_used_cycles[this].at(set * NUM_WAY + way) = current_cycle;
    }

    // Calculate the reward based on hit or miss
    double reward = hit ? 1.0 : -1.0;

    // Update the DQN agent with the state-action pair and reward
    std::vector<double> next_state = build_state(set, current_set, ip, full_addr, type);
    dqn_agent->remember(last_state, way, reward, next_state);  // Store experience

    // Perform experience replay to learn from the past experiences
    dqn_agent->replay();
}

// Final statistics after simulation
void CACHE::replacement_final_stats() {
    // Output final statistics for DQN (if needed)
    std::cout << "Final statistics for DQN replacement:" << std::endl;
    // Example: Print some performance metrics (e.g., miss rates)
}

// Helper method to build the state vector for the DQN agent
std::vector<double> CACHE::build_state(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    std::vector<double> state = {
        static_cast<double>(set),                      // Cache set index
        static_cast<double>(current_set->get_last_access_time()),  // Last access time
        static_cast<double>(ip % NUM_WAY),             // Some derived feature from the IP (can be modified)
        static_cast<double>(full_addr),                // Full address
        static_cast<double>(type)                      // Access type (read/write)
    };

    return state;
}
