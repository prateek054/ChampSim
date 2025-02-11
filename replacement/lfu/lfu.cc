#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

#include "cache.h"

namespace {
    // Map to track frequency counters for each cache line in each cache instance
    std::map<CACHE*, std::vector<uint64_t>> access_frequencies;
}

void CACHE::initialize_replacement() {
    // Initialize the frequency counter array for each cache line
    ::access_frequencies[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY, 0);
}

uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // Calculate the range of cache lines within the set
    auto begin = std::next(std::begin(::access_frequencies[this]), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    // Find the cache line with the lowest access frequency
    auto victim = std::min_element(begin, end);
    assert(begin <= victim);
    assert(victim < end);

    // Return the index of the chosen victim line
    return static_cast<uint32_t>(std::distance(begin, victim));  // cast protected by prior asserts
}

void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit) {
    // Increment the frequency counter for the accessed cache line
    if (!hit || access_type{type} != access_type::WRITE) {  // Skip this for writeback hits
        ::access_frequencies[this].at(set * NUM_WAY + way)++;
    }
}

void CACHE::replacement_final_stats() {
    // Any final statistics processing can go here, if needed
}