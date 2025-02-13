#include "ehc.h"
#include <algorithm>
#include <numeric>
#include <iostream>

// Constructor: Initialize EHC structures with correct HHT size
ehc::ehc(CACHE* cache) : replacement(cache)
{
    long NUM_WAY_LOCAL = cache->NUM_WAY;
    long NUM_SET_LOCAL = cache->NUM_SET;
    long TOTAL_BLOCKS = NUM_SET * NUM_WAY;  // Correct HHT size

    current_hit_counters.resize(NUM_SET_LOCAL, std::vector<uint8_t>(NUM_WAY_LOCAL, 0));
    further_expected_hits.resize(NUM_SET_LOCAL, std::vector<float>(NUM_WAY_LOCAL, 1));
    last_used_cycles.resize(TOTAL_BLOCKS, 0);
    hit_history_table.resize(TOTAL_BLOCKS);  // Allocate HHT for all blocks

    std::cout << "[EHC] Initialized with " << TOTAL_BLOCKS << " HHT entries." << std::endl;
}

// Find a victim block based on Expected Hit Count (EHC) policy
long ehc::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type)
{
    long victim = 0;
    float min_expected_hits = std::numeric_limits<float>::max(); // Initialize with a large value

    for (long way = 0; way < NUM_WAY; way++) {
        champsim::address block_addr = current_set[way].address;  // Get block address

        // Calculate Expected Future Hits (EFH) counter
        float expected_further_hits = further_expected_hits[set][way];

        // Select the way with the minimum expected further hits as the victim
        if (expected_further_hits < min_expected_hits) {
            min_expected_hits = expected_further_hits;
            victim = way;
        }
    }

    // Get victim block's address before eviction
    champsim::address victim_addr = current_set[victim].address;

    uint8_t victim_current_hits = current_hit_counters[set][victim];

    std::cout << "[EHC] Selected Victim -> Set: " << set << ", Way: " << victim 
              << ", Expected Further Hits: " << min_expected_hits 
              << ", Victim Addr: " << std::hex << victim_addr 
              << ", Current Hits: " << std::dec << static_cast<int>(victim_current_hits) << std::endl;

    // Update Hit History Table (HHT)
    int hht_index = find_hht_entry(victim_addr);
    if (hht_index != -1) {
            std::rotate(hit_history_table[hht_index].hit_count_queue.rbegin(),
                        hit_history_table[hht_index].hit_count_queue.rbegin() + 1,
                        hit_history_table[hht_index].hit_count_queue.rend());
            hit_history_table[hht_index].hit_count_queue[0] = current_hit_counters[set][victim];
    } 
    if (hht_index != -1) {
        // Print HHT entry details
        HHTEntry& hht_entry = hit_history_table[hht_index];

        std::cout << "[EHC] HHT Entry -> Index: " << hht_index
                  << ", Valid: " << hht_entry.valid
                  << ", Tag: " << std::hex << hht_entry.tag
                  << ", Hit Count Queue: { ";

        for (int count : hht_entry.hit_count_queue) {
            std::cout << static_cast<int>(count) << " ";
        }

        std::cout << "}" << std::dec << std::endl;

        // Rotate the hit count queue (move all elements right)
        std::rotate(hht_entry.hit_count_queue.rbegin(),
                    hht_entry.hit_count_queue.rbegin() + 1,
                    hht_entry.hit_count_queue.rend());

        // Update the most recent hit count
        hht_entry.hit_count_queue[0] = current_hit_counters[set][victim];
    } 

     current_hit_counters[set][victim] = 0;
     further_expected_hits[set][victim] = 0;

    return victim;
}


// Update replacement state (called on cache accesses)
void ehc::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit)
{
    if (hit) {
        if (current_hit_counters[set][way] < 7)  // Max 3-bit counter
            current_hit_counters[set][way]++;

        if (further_expected_hits[set][way] > 0) {
            further_expected_hits[set][way]--;
        }
        std::cout << "[EHC] Hit on Set " << set << ", Way " 
                  << way << " (New Hit Count: " << current_hit_counters[set][way] 
                  << "Expect further hits " << further_expected_hits[set][way] << ")" << std::endl;
    } else {

        std::cout << "[EHC] not hit in replacement state" << std::endl;
        // do nothhing
       // replacement_cache_fill(triggering_cpu, set, way, full_addr, ip, victim_addr, type);
    }

    // Update last used cycle
    last_used_cycles[set * NUM_WAY + way] = cycle;
}

// Handle cache fills (new block insertions)
//void ehc::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr,
  //                               uint32_t type) 
void ehc::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, 
                                 champsim::address victim_addr, access_type type)
{
    std::cout << "[EHC] Cache fill at Set " << set << ", Way " << way << " with Addr: " << std::hex << full_addr << std::dec << std::endl;
    current_hit_counters[set][way] = 0; // Reset hit counter for new block

    int hht_index = find_hht_entry(full_addr);

    // If entry exists in HHT, update its hit count queue
    if (hht_index != -1) {
        std::rotate(hit_history_table[hht_index].hit_count_queue.rbegin(),
                    hit_history_table[hht_index].hit_count_queue.rbegin() + 1,
                    hit_history_table[hht_index].hit_count_queue.rend());
        
        // Compute the expected hit count as the average of the last 4 values
        float avg_hit_count = 0;
        for (float count : hit_history_table[hht_index].hit_count_queue) {
            avg_hit_count += count;
        }
        avg_hit_count /= hit_history_table[hht_index].hit_count_queue.size();

        // Update the expected hit counter for this set/way
        further_expected_hits[set][way] = avg_hit_count;

        std::cout << "[EHC] Updated Expected Hit Counter: " << avg_hit_count << " for Set " << set << ", Way " << way << std::endl;

    } else {
        // Insert new entry into HHT (replace least recently used entry)
        int replace_index = static_cast<int>(std::distance(hit_history_table.begin(),
                                                   std::min_element(hit_history_table.begin(), hit_history_table.end(),
                                                                    [](const HHTEntry &a, const HHTEntry &b) {
                                                                        return !a.valid || (b.valid && a.tag > b.tag);
                                                                    })));
        hit_history_table[replace_index] = {true, full_addr, {0, 0, 0, 0}};

        // Update the expected hit counter for this set/way
        further_expected_hits[set][way] = 1;
    }
}

// Final replacement statistics (optional logging)
//void ehc::replacement_final_stats() {
  //  std::cout << "[EHC] Final Statistics: Simulation Complete." << std::endl;
//}

// Find HHT entry based on tag (search entire HHT, since it's block-based now)
/*int ehc::find_hht_entry(uint64_t tag) {
    for (int i = 0; i < HHT_ENTRIES; i++) {
        if (hit_history_table[i].valid && hit_history_table[i].tag == tag)
            return i;
    }
    return -1;
}*/

int ehc::find_hht_entry(champsim::address full_addr) 
{
    //uint64_t tag = full_addr >> LOG2_BLOCK_SIZE; // Extract tag from full address

    // Search for an existing entry in the HHT
    for (int i = 0; i < HHT_ENTRIES; i++) {
        if (hit_history_table[i].valid && hit_history_table[i].tag == full_addr) {
            return i; // Return index if entry is found
        }
    }

    return -1; // Return -1 if not found
}

