// q_learning_replacement.cc

#include "cache.h"
#include <map>
#include <utility>
#include <cmath>
#include <cstdlib>
#include <iostream>

#define DEBUG true
#define DEBUG_PRINT(x) do { if (DEBUG) std::cout << x << std::endl; } while (0)

class Q_LearningReplacement {
public:
    Q_LearningReplacement(int num_lines);
    int find_victim(CacheBlock *cache, int num_lines);
    void update_q_table(int state, int action, float reward, int next_state);

private:
    int get_state(CacheBlock *cache, int num_lines);
    int choose_action(int state, int num_lines);

    float alpha = 0.1;    // Learning rate
    float gamma = 0.9;    // Discount factor
    float epsilon = 0.1;  // Exploration rate
    int cache_size;

    std::map<std::pair<int, int>, float> Q_table; // Q-table: (state, action) -> Q-value
};

Q_LearningReplacement::Q_LearningReplacement(int num_lines) : cache_size(num_lines) {}

// Function to compute the current state based on cache occupancy
int Q_LearningReplacement::get_state(CacheBlock *cache, int num_lines) {
    int occupancy = 0;
    for (int i = 0; i < num_lines; i++) {
        if (cache[i].valid)
            occupancy++;
    }
    DEBUG_PRINT("Current State (Occupancy): " << occupancy);
    return occupancy; // Example state based on cache occupancy
}

// Function to choose an action based on epsilon-greedy strategy
int Q_LearningReplacement::choose_action(int state, int num_lines) {
    if ((float)rand() / RAND_MAX < epsilon) {
        // Exploration: choose a random action
        int random_action = rand() % num_lines;
        DEBUG_PRINT("Choosing Random Action: " << random_action);
        return random_action;
    } else {
        // Exploitation: choose the action with max Q-value
        int best_action = 0;
        float max_q_value = -INFINITY;
        for (int action = 0; action < num_lines; action++) {
            auto q_value = Q_table[{state, action}];
            if (q_value > max_q_value) {
                max_q_value = q_value;
                best_action = action;
            }
        }
        DEBUG_PRINT("Choosing Best Action: " << best_action << " with Q-value: " << max_q_value);
        return best_action;
    }
}

// Function to update Q-table based on the reward received
void Q_LearningReplacement::update_q_table(int state, int action, float reward, int next_state) {
    float old_q_value = Q_table[{state, action}];
    float max_next_q_value = -INFINITY;
    for (int a = 0; a < cache_size; a++) {
        max_next_q_value = std::max(max_next_q_value, Q_table[{next_state, a}]);
    }
    Q_table[{state, action}] = old_q_value + alpha * (reward + gamma * max_next_q_value - old_q_value);

    DEBUG_PRINT("Updating Q-table for State: " << state << ", Action: " << action);
    DEBUG_PRINT("Old Q-value: " << old_q_value << ", Reward: " << reward << ", Max Next Q-value: " << max_next_q_value);
    DEBUG_PRINT("New Q-value: " << Q_table[{state, action}]);
}

// Main eviction function using Q-learning
int Q_LearningReplacement::find_victim(CacheBlock *cache, int num_lines) {
    // Get the current state
    int state = get_state(cache, num_lines);

    // Choose an action based on the current state
    int action = choose_action(state, num_lines);

    // Reward function (hit = +1, miss = -1)
    float reward = cache[action].valid ? -1.0 : 1.0;
    DEBUG_PRINT("Action: " << action << ", Reward: " << reward);

    // Update Q-table with the new state and reward
    int next_state = get_state(cache, num_lines);
    update_q_table(state, action, reward, next_state);

    DEBUG_PRINT("Selected Victim Cache Line Index: " << action);
    return action; // Return the chosen victim cache line index
}

// Register this replacement policy with ChampSim
extern "C" {
    Q_LearningReplacement* make_replacement_policy(int num_lines) {
        return new Q_LearningReplacement(num_lines);
    }
}
