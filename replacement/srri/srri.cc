#include "srri.h"
#include <algorithm>
#include <numeric>
#include <iostream>

srri::srri(CACHE* cache)
    : replacement(cache),
      rri_table(NUM_SET, std::vector<RRIEntry>(NUM_WAY)),
      global_cycle(0)
{
    long TOTAL_BLOCKS = NUM_SET * NUM_WAY;
    std::cout << "[SRRI-LLC] Initialized with " << TOTAL_BLOCKS << " RRI entries." << std::endl;
}

float srri::predict_rri(const std::vector<std::vector<uint64_t>>& rri_history) const {
    std::cout << "[SRRI-LLC] Predicting RRI..." << std::endl;
    std::cout << "[SRRI-LLC] rri_history has " << rri_history.size() << " rows." << std::endl;

    for (size_t i = 0; i < rri_history.size(); ++i) {
        std::cout << "[SRRI-LLC] Row " << i << ": ";
        for (auto val : rri_history[i]) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    int count = 2;
    int size = rri_history.size();
    int start = size >= count ? size - count : 0;

    std::vector<double> row_averages;

    for (int i = start; i < size; ++i) {
        const auto& row = rri_history[i];
        if (!row.empty()) {
            double sum = std::accumulate(row.begin(), row.end(), uint64_t(0));
            row_averages.push_back(sum / row.size());
        }
    }

    if (row_averages.empty()) {
        std::cout << "[SRRI-LLC] No row averages to calculate, returning 0.0\n";
        return 0.0f;
    }

    double final_avg = std::accumulate(row_averages.begin(), row_averages.end(), 0.0) / row_averages.size();
    std::cout << "[SRRI-LLC] Final predicted RRI = " << final_avg << std::endl;

    return static_cast<float>(final_avg);
}

long srri::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set,
                       const champsim::cache_block* current_set, champsim::address ip,
                       champsim::address full_addr, access_type type)
{
    long victim = 0;
    float max_predicted_rri = -1.0f;

    std::cout << "[SRRI-LLC] Finding victim for set " << set << std::endl;

    for (long way = 0; way < NUM_WAY; ++way) {
        champsim::address block_addr = current_set[way].address;
        int hht_index = find_rri_entry(block_addr);

        if (hht_index != -1) {
            RRIEntry& rri_entry = hit_rri_table[hht_index];
            float predicted_rri = predict_rri(rri_entry.rri_history);
            std::cout << "[SRRI-LLC] Way " << way << ", Predicted RRI: " << predicted_rri << std::endl;

            if (predicted_rri > max_predicted_rri) {
                max_predicted_rri = predicted_rri;
                victim = way;
            }
        } else {
            std::cout << "[SRRI-LLC] Block address not found in HHT. Creating new entry.\n";
            hit_rri_table.push_back({true, full_addr, {{0}}});
        }
    }

    std::cout << "[SRRI-LLC] Selected victim = Way " << victim << std::endl;
    return victim;
}

void srri::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                  champsim::address ip, champsim::address victim_addr, access_type type)
{
    auto& entry = rri_table[set][way];
    global_cycle++;
    std::cout << "[SRRI-LLC] Cache fill at set " << set << ", way " << way << std::endl;

    int hht_index = find_rri_entry(full_addr);

    if (hht_index != -1) {
        RRIEntry& rri_entry = hit_rri_table[hht_index];

        if (rri_entry.rri_history.empty() || !rri_entry.rri_history.back().empty()) {
            std::cout << "[SRRI-LLC] Appending new empty row to rri_history\n";
            rri_entry.rri_history.push_back({});
        }

        std::cout << "[SRRI-LLC] Appending 0 to the last row\n";
        rri_entry.rri_history.back().push_back(0);
    } else {
        std::cout << "[SRRI-LLC] Creating new HHT entry for address\n";
        hit_rri_table.push_back({true, full_addr, {{0}}});
    }
}

void srri::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                    champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    auto& entry = rri_table[set][way];
    global_cycle++;

    if (hit) {
        std::cout << "[SRRI-LLC] Hit update at set " << set << ", way " << way << std::endl;

        int hht_index = find_rri_entry(full_addr);

        if (hht_index != -1) {
            RRIEntry& rri_entry = hit_rri_table[hht_index];

            if (!rri_entry.rri_history.empty()) {
                std::cout << "[SRRI-LLC] Appending RRI (" << way << ") to last row\n";
                rri_entry.rri_history.back().push_back(way);
            } else {
                std::cout << "[SRRI-LLC] Warning: trying to append to empty history. Creating new row.\n";
                rri_entry.rri_history.push_back({way});
            }
        }
    }
}

int srri::find_rri_entry(champsim::address full_addr) 
{
    if (hit_rri_table.empty()) {
        std::cout << "[SRRI-LLC] HHT is empty.\n";
        return -1;
    }

    for (size_t i = 0; i < hit_rri_table.size(); i++) {
        if (hit_rri_table[i].tag == full_addr) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
