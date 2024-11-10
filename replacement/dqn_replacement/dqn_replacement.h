#ifndef DQN_REPLACEMENT_H
#define DQN_REPLACEMENT_H

#include <vector>
#include <map>
#include <cstdint>
#include "cache.h"
#include "dqn_agent.h"  // DQN Agent header, which contains the DQN implementation

// DQN Replacement class for cache replacement
class DQNReplacement {
public:
    DQNReplacement();  // Constructor to initialize the agent and other members
    ~DQNReplacement(); // Destructor to clean up memory
    void initialize_replacement(CACHE* cache); // Initialize the replacement strategy
    uint32_t find_victim(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type);  // Select a victim for eviction
    void update_replacement_state(uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint32_t type, uint8_t hit);  // Update the state after accessing a cache line
    void replacement_final_stats(); // Final statistics after simulation

private:
    DQNAgent* dqn_agent; // The DQN agent responsible for choosing the victim
    std::map<CACHE*, std::vector<uint64_t>> last_used_cycles;  // Track last access times (used by some features)
    CACHE* associated_cache;  // Reference to the cache this replacement policy applies to
};

#endif // DQN_REPLACEMENT_H

