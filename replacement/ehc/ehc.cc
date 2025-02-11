#include <algorithm>
#include <cassert>
#include <map>
#include <vector>
#include <array>
#include <limits>

#include "cache.h"

namespace
{
    constexpr uint32_t HHT_ENTRIES = 2048;   // Number of entries in Hit History Table
    constexpr uint32_t HISTORY_LENGTH = 4;  // Last 4 hit counts stored

    // Global structures for tracking cache access history
    std::map<CACHE*, std::vector<uint32_t>> hit_counters; // Hit counters for each block
    std::map<CACHE*, std::vector<uint64_t>> last_used_cycles; // Last access cycles

    // Structure for Hit History Table (HHT)
    struct HHTEntry {
        bool valid = false;
        uint64_t tag = 0;
        std::array<uint8_t, HISTORY_LENGTH> hit_count_queue = {0, 0, 0, 0}; // FIFO queue of past hit counts
    };

    // Hit History Table (HHT) - Stores past access counts
    std::map<CACHE*, std::vector<HHTEntry>> hit_history_table;
}

// Initialize EHC metadata
void CACHE::initialize_replacement() {
    ::last_used_cycles[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY);
    ::hit_counters[this] = std::vector<uint32_t>(NUM_SET * NUM_WAY, 0);
    ::hit_history_table[this] = std::vector<HHTEntry>(HHT_ENTRIES);
}

// Find a victim block based on EHC policy
uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    auto begin = std::next(std::begin(::hit_counters[this]), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    uint32_t victim = 0;
    float lowest_score = std::numeric_limits<float>::max();

    for (uint32_t way = 0; way < NUM_WAY; way++) {
        uint8_t expected_hits = compute_expected_hits(set, way, full_addr);
        float score = static_cast<float>(expected_hits) / (hit_counters[this][set * NUM_WAY + way] + 1);  // Expected Hits / (Current Hits + 1)

        if (score < lowest_score) {
            lowest_score = score;
            victim = way;
        }
    }

    return victim;
}

// Update cache replacement metadata
void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit) {
    uint32_t index = set * NUM_WAY + way;

    // If a hit occurs, increment the hit counter
    if (hit) {
        if (hit_counters[this][index] < 7)  // 3-bit counter max value = 7
            hit_counters[this][index]++;
    }
    else {
        // Eviction: Update HHT with past hit count
        int hht_index = find_hht_entry(full_addr);
        if (hht_index != -1) {
            // Found entry, update FIFO queue
            std::rotate(hit_history_table[this][hht_index].hit_count_queue.rbegin(),
                        hit_history_table[this][hht_index].hit_count_queue.rbegin() + 1,
                        hit_history_table[this][hht_index].hit_count_queue.rend());
            hit_history_table[this][hht_index].hit_count_queue[0] = hit_counters[this][index];
        } else {
            // Insert new entry into HHT (replace LRU entry)
            int replace_index = std::distance(hit_history_table[this].begin(),
                                              std::min_element(hit_history_table[this].begin(), hit_history_table[this].end(),
                                                               [](const HHTEntry &a, const HHTEntry &b) {
                                                                   return !a.valid || (b.valid && a.tag > b.tag);
                                                               }));
            hit_history_table[this][replace_index] = {true, full_addr, {hit_counters[this][index], 0, 0, 0}};
        }

        // Reset hit counter for the new block
        hit_counters[this][index] = 0;
    }

    // Update last used cycle
    if (!hit || access_type{type} != access_type::WRITE)  // Skip writeback hits
        ::last_used_cycles[this][index] = current_cycle;
}

// Compute expected hit count from HHT
uint8_t CACHE::compute_expected_hits(uint32_t set, uint32_t way, uint64_t full_addr) {
    int hht_index = find_hht_entry(full_addr);
    if (hht_index != -1) {
        auto &queue = hit_history_table[this][hht_index].hit_count_queue;
        return std::accumulate(queue.begin(), queue.end(), 0) / HISTORY_LENGTH;  // Average of last 4 hit counts
    }
    return 1;  // Default expected hit count if no history exists
}

// Find an entry in HHT by tag
int CACHE::find_hht_entry(uint64_t full_addr) {
    for (int i = 0; i < HHT_ENTRIES; i++) {
        if (hit_history_table[this][i].valid && hit_history_table[this][i].tag == full_addr)
            return i;
    }
    return -1;
}

// No additional final stats needed
void CACHE::replacement_final_stats() {}
