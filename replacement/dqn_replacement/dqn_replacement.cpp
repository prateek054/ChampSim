#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

#include "cache.h"
#include "dqn_agent.h"  // Include DQN Agent header

namespace {
    // Map to track access frequencies for each cache line in each cache instance
    std::map<CACHE*, std::vector<uint64_t>> access_frequencies;
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

// Call the original initialize_replacement method from CACHE
void CACHE::initialize_replacement() {
    // Initialize the frequency counter array for each cache line
    ::access_frequencies[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY, 0);
}

// Find victim cache line using DQN logic, calling the original find_victim method from CACHE
uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Calculate the range of cache lines within the set
    auto begin = std::next(std::begin(::access_frequencies[this]), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    // DQN logic: Use the DQN agent to decide which cache line to evict
    std::vector<double> state = build_state(set, current_set, ip, full_addr, type);  // Build the state vector
    int victim_way = dqn_agent->act(state);  // The agent chooses a victim based on the state

    // Find the cache line with the lowest access frequency (fallback to LRU-like behavior if needed)
    if (victim_way < 0 || victim_way >= NUM_WAY) {
        auto victim = std::min_element(begin, end);  // Fallback to using frequency if DQN doesn't choose a valid victim
        assert(begin <= victim);
        assert(victim < end);
        victim_way = static_cast<uint32_t>(std::distance(begin, victim));  // cast protected by prior asserts
    }

    std::cout << "DQN selected victim at way: " << victim_way << std::endl;
    return victim_way;  // Return the victim cache line (way)
}

// Update the replacement state after an access
void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit) {
    // Increment the frequency counter for the accessed cache line, using DQN-based logic for updates
    if (!hit || access_type{type} != access_type::WRITE) {  // Skip this for writeback hits
        ::access_frequencies[this].at(set * NUM_WAY + way)++;  // Update the frequency counter
    }

    // The DQN agent learns from the cache access
    double reward = hit ? 1.0 : -1.0;  // Reward based on hit or miss
    std::vector<double> next_state = build_state(set, current_set, ip, full_addr, type);  // Build the next state
    dqn_agent->remember(last_state, way, reward, next_state);  // Store the experience
    dqn_agent->replay();  // Perform experience replay to improve decision-making
}

// Final statistics after simulation
void CACHE::replacement_final_stats() {
    std::cout << "Final statistics for DQN replacement:" << std::endl;
    // Example: Print performance metrics such as the number of evictions or misses
}

// Helper method to build the state vector for the DQN agent
std::vector<double> CACHE::build_state(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    std::vector<double> state = {
        static_cast<double>(set),                      // Cache set index
        static_cast<double>(current_set->get_last_access_time()),  // Last access time of the current block
        static_cast<double>(ip % NUM_WAY),             // Some derived feature from the IP (can be modified)
        static_cast<double>(full_addr),                // Full address of the accessed cache line
        static_cast<double>(type)                      // Access type (read/write)
    };

    return state;
}
