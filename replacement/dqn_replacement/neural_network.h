#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <vector>

class NeuralNetwork {
public:
    NeuralNetwork(int input_size, int output_size);
    std::vector<double> forward(const std::vector<double>& input);
    void train(const std::vector<double>& input, const std::vector<double>& target);

private:
    int input_size;
    int output_size;
    std::vector<std::vector<double>> weights;
    std::vector<double> bias;
};

#endif // NEURAL_NETWORK_H
