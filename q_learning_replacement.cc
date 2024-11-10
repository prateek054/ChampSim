#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include "cache.h"

namespace
{
    // Define Q-learning structures and parameters
    std::map<std::pair<uint32_t, uint32_t>, float> Q_table;
    float alpha = 0.1;     // Learning rate
    float gama = 0.9;     // Discount factor
    float epsilon = 0.1;   // Exploration rate
}

// Initialize replacement policy (Q-learning setup)
void CACHE::initialize_replacement() {
    std::cout << "Initializing Q-learning Replacement Policy..." << std::endl;
}

// Find victim cache line using Q-learning
uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Step 1: Determine the current state of the cache set
    uint32_t state = 0;
    for (uint32_t i = 0; i < NUM_WAY; ++i) {
        if (current_set[i].valid)
            ++state; // For example, state can represent cache occupancy
    }
   // std::cout << "Current state (occupancy): " << state << std::endl;

    // Step 2: Choose an action (cache way to evict) using epsilon-greedy strategy
    uint32_t action;
    if (static_cast<float>(rand()) / RAND_MAX < epsilon) {
        // Exploration: choose a random action
        action = rand() % NUM_WAY;
     //   std::cout << "Choosing Random Action: " << action << std::endl;
    } else {
        // Exploitation: choose the action with the maximum Q-value
        float max_q_value = -INFINITY;
        action = 0;
        for (uint32_t i = 0; i < NUM_WAY; ++i) {
            float q_value = Q_table[{state, i}];
            if (q_value > max_q_value) {
                max_q_value = q_value;
                action = i;
            }
        }
       // std::cout << "Choosing Best Action: " << action << " with Q-value: " << Q_table[{state, action}] << std::endl;
    }

    // Return the chosen cache line to evict
    //std::cout << "Chosen victim way: " << action << " for set: " << set << std::endl;
    return action;
}

// Update replacement state using Q-learning
void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit) {
    // Step 1: Get the current state
    uint32_t state = 0;
    for (uint32_t i = 0; i < NUM_WAY; ++i) {
        if (this->block[set * NUM_WAY + i].valid)
            ++state; // Occupancy-based state representation
    }
    //std::cout << "Current state (occupancy): " << state << std::endl;

    // Step 2: Calculate the reward
    float reward = hit ? 1.0 : -1.0;
   // std::cout << "Reward: " << reward << std::endl;

    // Step 3: Get the next state after the action
    uint32_t next_state = 0;
    for (uint32_t i = 0; i < NUM_WAY; ++i) {
        if (this->block[set * NUM_WAY + i].valid)
            ++next_state;
    }
    //std::cout << "Next state (occupancy): " << next_state << std::endl;

    // Step 4: Update the Q-value for the (state, action) pair
    float max_next_q_value = -INFINITY;
    for (uint32_t i = 0; i < NUM_WAY; ++i) {
        max_next_q_value = std::max(max_next_q_value, Q_table[{next_state, i}]);
    }
    float& current_q_value = Q_table[{state, way}];
    current_q_value = current_q_value + alpha * (reward + gama * max_next_q_value - current_q_value);

    //std::cout << "Updated Q-value for (state: " << state << ", action: " << way << "): " << current_q_value << std::endl;
}
void CACHE::replacement_final_stats() {}
