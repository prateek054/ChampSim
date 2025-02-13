#include "lru.h"
#include <algorithm>
#include <cassert>
#include <fmt/core.h> // Include fmt for formatted printing

lru::lru(CACHE* cache) : lru(cache, cache->NUM_SET, cache->NUM_WAY) {
    fmt::print("[LRU] Constructor: Initialized LRU for {} sets and {} ways.\n", cache->NUM_SET, cache->NUM_WAY);
}

lru::lru(CACHE* cache, long sets, long ways) 
    : replacement(cache), NUM_WAY(ways), last_used_cycles(static_cast<std::size_t>(sets * ways), 0) 
{
    fmt::print("[LRU] Constructor: Custom Initialization with {} sets and {} ways.\n", sets, ways);
}

long lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, 
                      champsim::address ip, champsim::address full_addr, access_type type)
{
    auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    // Find the way whose last use cycle is most distant (LRU block)
    auto victim = std::min_element(begin, end);
    assert(begin <= victim);
    assert(victim < end);

    long victim_way = std::distance(begin, victim);
    fmt::print("[LRU] find_victim: CPU {} | Instr {} | Set {} | Victim Way {} | Addr: {:x} | Access Type: {}\n", 
               triggering_cpu, instr_id, set, victim_way, full_addr, static_cast<int>(type));

    return victim_way;
}

void lru::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, 
                                 champsim::address victim_addr, access_type type)
{
    // Mark the way as being used on the current cycle
    last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
    
    fmt::print("[LRU] replacement_cache_fill: CPU {} | Set {} | Way {} | Addr: {:x} | Victim Addr: {:x} | Access Type: {}\n",
               triggering_cpu, set, way, full_addr, victim_addr, static_cast<int>(type));
}

void lru::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type, uint8_t hit)
{
    // Mark the way as being used on the current cycle (except for writeback hits)
    if (hit && access_type{type} != access_type::WRITE) {
        last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
    }

    fmt::print("[LRU] update_replacement_state: CPU {} | Set {} | Way {} | Addr: {:x} | Hit: {} | Access Type: {}\n",
               triggering_cpu, set, way, full_addr, hit, static_cast<int>(type));
}
