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

float srri::predict_rri(const std::array<uint64_t, HISTORY_LENGTH>& history) const {
    uint64_t sum = 0;
    int count = 0;
    for (auto rri : history) {
        if (rri > 0) {
            sum += rri;
            count++;
        }
    }
    return (count > 0) ? static_cast<float>(sum) / count : std::numeric_limits<float>::infinity();
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
    entry.valid = true;
    entry.tag = full_addr;
    entry.rri_history.fill(0);
    entry.last_access_cycle = global_cycle;
}

void srri::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                    champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    auto& entry = rri_table[set][way];
    global_cycle++;

    if (hit && entry.valid) {
        uint64_t rri = way; // As requested
        for (int i = HISTORY_LENGTH - 1; i > 0; --i)
            entry.rri_history[i] = entry.rri_history[i - 1];
        entry.rri_history[0] = rri;
        entry.last_access_cycle = global_cycle;
    }
}
