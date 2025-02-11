#ifndef REPLACEMENT_EHC_H
#define REPLACEMENT_EHC_H

#include <vector>
#include <array>
#include "cache.h"
#include "modules.h"

constexpr long NUM_SET = 2048;  // Number of sets
constexpr long NUM_WAY = 16;    // Associativity (adjustable)
constexpr long HHT_ENTRIES = NUM_SET * NUM_WAY;  // Total cache blocks

constexpr long HISTORY_LENGTH = 4;  // Number of past hit counts stored

class ehc : public champsim::modules::replacement
{

private: 
  std::vector<std::array<uint32_t, HISTORY_LENGTH>> hit_counters; // Per-block hit counters
  std::vector<uint64_t> last_used_cycles;
  uint64_t cycle = 0;

  struct HHTEntry {
      bool valid = false;
      uint64_t tag = 0;
      std::array<uint8_t, HISTORY_LENGTH> hit_count_queue = {0, 0, 0, 0}; // FIFO queue of past hit counts
  };

  std::vector<HHTEntry> hit_history_table;  // HHT now stores per-block data
  std::vector<std::vector<uint8_t>> current_hit_counters;   // Track hits per block
  std::vector<std::vector<float>> further_expected_hits;    // Stores expected hits per block

  int find_hht_entry(uint64_t tag);  // <---- Add this declaration


public:
  explicit ehc(CACHE* cache);
  ehc(CACHE* cache, long sets, long ways);

  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, 
                   uint64_t ip, uint64_t full_addr, uint32_t type);

  void update_replacement_state(uint32_t triggering_cpu, long set, long way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr,
                                uint32_t type, uint8_t hit);

  void replacement_final_stats();

  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr,
                              uint32_t type);
};

#endif // REPLACEMENT_EHC_H
