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

/**float row_average(const std::vector<uint64_t>& row) {
    if (row.empty()) return 0.0;
    return static_cast<float>(std::accumulate(row.begin(), row.end(), uint64_t(0))) / row.size();
}*/

//float srri::predict_rri(const std::array<uint64_t, HISTORY_LENGTH>& history) const { 

/** uint64_t sum = 0;
    int count = 0;
    for (auto rri : history) {
        if (rri > 0) {
            sum += rri;
            count++;
        }
    }
    return (count > 0) ? static_cast<float>(sum) / count : std::numeric_limits<float>::infinity();

    */
//}

//  to calculate the average of the last 3 rows of a 2D vector and 
// then get the average of those averages
float srri::predict_rri(const std::vector<std::vector<uint64_t>>& rri_history) const {
    int count = 3;
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

    if (row_averages.empty()) return 0.0f;

    double final_avg = std::accumulate(row_averages.begin(), row_averages.end(), 0.0) / row_averages.size();

    return static_cast<float>(final_avg);
}


long srri::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set,
                       const champsim::cache_block* current_set, champsim::address ip,
                       champsim::address full_addr, access_type type)
{
    long victim = 0;
    float max_predicted_rri = -1.0f;

    for (long way = 0; way < NUM_WAY; ++way) {
        auto& entry = rri_table[set][way];
        if (!entry.valid) return way; // Choose empty slot immediately

        float predicted_rri = predict_rri(entry.rri_history);
        if (predicted_rri > max_predicted_rri) {
            max_predicted_rri = predicted_rri;
            victim = way;
        }
    }
    return victim;
}

void srri::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                  champsim::address ip, champsim::address victim_addr, access_type type)
{
    auto& entry = rri_table[set][way];
    global_cycle++;

    if (!entry.rri_history.empty()) {
        entry.rri_history.back().push_back(0); // Appends 77 to the last row
}   else {
        // If it's empty, you can choose to create the first row
        entry.rri_history.push_back({0});
    }
}

void srri::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                    champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    auto& entry = rri_table[set][way];
    global_cycle++;

    if (hit && entry.valid) {
        uint64_t rri = way; // As requested
       // for (int i = HISTORY_LENGTH - 1; i > 0; --i)
         //   entry.rri_history[i] = entry.rri_history[i - 1];
        //entry.rri_history[0] = rri;
        entry.rri_history.back().push_back(way);
        entry.last_access_cycle = global_cycle;
    }
}
