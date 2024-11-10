#include "neural_network.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>

NeuralNetwork::NeuralNetwork(int input_size, int output_size)
    : input_size(input_size), output_size(output_size) {
    // Initialize weights randomly
    srand(time(0));
    weights.resize(input_size, std::vector<double>(output_size));
    bias.resize(output_size, 0.0);

    for (int i = 0; i < input_size; ++i) {
        for (int j = 0; j < output_size; ++j) {
            weights[i][j] = static_cast<double>(rand()) / RAND_MAX;
        }
    }
}

std::vector<double> NeuralNetwork::forward(const std::vector<double>& input) {
    std::vector<double> output(output_size, 0.0);

    for (int i = 0; i < input_size; ++i) {
        for (int j = 0; j < output_size; ++j) {
            output[j] += input[i] * weights[i][j];
        }
    }

    // Add biases
    for (int j = 0; j < output_size; ++j) {
        output[j] += bias[j];
    }

    return output;  // Simple forward pass (no activation function for simplicity)
}

void NeuralNetwork::train(const std::vector<double>& input, const std::vector<double>& target) {
    // Simple gradient descent for training (dummy implementation)
    double learning_rate = 0.01;

    std::vector<double> output = forward(input);

    // Calculate error and adjust weights (no backpropagation here, simple gradient descent)
    for (int i = 0; i < input_size; ++i) {
        for (int j = 0; j < output_size; ++j) {
            weights[i][j] -= learning_rate * (output[j] - target[j]) * input[i];
        }
    }

    // Update biases
    for (int j = 0; j < output_size; ++j) {
        bias[j] -= learning_rate * (output[j] - target[j]);
    }
}
