#include "dqn_agent.h"
#include "neural_network.h"
#include <random>
#include <iostream>
#include <algorithm>

DQNAgent::DQNAgent(int num_actions, int state_size)
    : num_actions(num_actions), state_size(state_size), exploration_rate(1.0), exploration_decay(0.995), exploration_min(0.01),
      q_network(state_size, num_actions) {  // Initialize Q-network
}

int DQNAgent::act(const std::vector<double>& state) {
    if (static_cast<double>(rand()) / RAND_MAX <= exploration_rate) {
        // Exploration: Select a random action
        return rand() % num_actions;
    } else {
        // Exploitation: Select the action with the highest Q-value
        std::vector<double> q_values = q_network.forward(state);
        int best_action = 0;
        for (int i = 1; i < num_actions; ++i) {
            if (q_values[i] > q_values[best_action]) {
                best_action = i;
            }
        }
        return best_action;
    }
}

void DQNAgent::remember(const std::vector<double>& state, int action, double reward, const std::vector<double>& next_state) {
    memory.push_back(std::make_tuple(state, action, reward, next_state));
    if (memory.size() > 10000) {
        memory.erase(memory.begin());  // Keep the buffer size manageable
    }
}

void DQNAgent::replay() {
    if (memory.size() < 64) return;  // Not enough experiences to replay

    // Randomly sample a batch from memory
    std::random_shuffle(memory.begin(), memory.end());
    for (int i = 0; i < 64; ++i) {
        auto [state, action, reward, next_state] = memory[i];

        // Get the Q-values for the current state and next state
        std::vector<double> q_values = q_network.forward(state);
        std::vector<double> next_q_values = q_network.forward(next_state);

        // Calculate the target Q-value for this experience
        double target = reward + 0.99 * *std::max_element(next_q_values.begin(), next_q_values.end());

        // Update the Q-value for the chosen action
        q_values[action] = target;

        // Train the Q-network on the updated Q-values
        q_network.train(state, q_values);
    }

    // Decay exploration rate
    if (exploration_rate > exploration_min) {
        exploration_rate *= exploration_decay;
    }
}

void DQNAgent::train_network(const std::vector<double>& state, const std::vector<double>& target) {
    // Train the Q-network based on the current state and target values (Q-learning)
    q_network.train(state, target);
}

void DQNAgent::save(const std::string& filename) {
    std::cout << "Saving model to " << filename << std::endl;
    // Implement model saving functionality (e.g., saving weights to a file)
}

void DQNAgent::load(const std::string& filename) {
    std::cout << "Loading model from " << filename << std::endl;
    // Implement model loading functionality (e.g., loading weights from a file)
}
