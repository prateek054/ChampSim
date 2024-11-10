#ifndef DQN_REPLACEMENT_H
#define DQN_REPLACEMENT_H

#include <vector>
#include <map>
#include <cstdint>
#include "cache.h"
#include "dqn_agent.h"  // Include DQN Agent header

// DQN Replacement class for cache replacement
class DQNReplacement : public CACHE {
public:
    DQNReplacement();  // Constructor to initialize the agent and other members
    ~DQNReplacement(); // Destructor to clean up memory

    // Override the initialize_replacement method from CACHE
    void initialize_replacement() override;

    // Override the find_victim method from CACHE
    uint32_t find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) override;

    void update_replacement_state(uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint32_t type, uint8_t hit);
    void replacement_final_stats();

private:
    DQNAgent* dqn_agent;  // The DQN agent responsible for choosing the victim
    std::map<CACHE*, std::vector<uint64_t>> last_used_cycles;  // Track last access times
    std::vector<double> build_state(uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type);  // Build state vector
};

#endif // DQN_REPLACEMENT_H
