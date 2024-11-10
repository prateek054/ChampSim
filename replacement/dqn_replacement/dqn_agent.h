#ifndef DQN_AGENT_H
#define DQN_AGENT_H

#include <vector>
#include <random>
#include <iostream>
#include "neural_network.h"  // Include the NeuralNetwork class header

// DQN Agent for Cache Replacement
class DQNAgent {
public:
    DQNAgent(int num_actions, int state_size);
    int act(const std::vector<double>& state);
    void remember(const std::vector<double>& state, int action, double reward, const std::vector<double>& next_state);
    void replay();
    void train_network();
    void save(const std::string& filename);
    void load(const std::string& filename);

private:
    int num_actions;
    int state_size;
    double exploration_rate;
    double exploration_decay;
    double exploration_min;
    std::vector<std::tuple<std::vector<double>, int, double, std::vector<double>>> memory;
    NeuralNetwork q_network;  // The Q-network
    void update_network(const std::vector<double>& state, const std::vector<double>& target);
};

#endif // DQN_AGENT_H
