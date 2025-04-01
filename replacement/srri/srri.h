#ifndef REPLACEMENT_SRRI_H
#define REPLACEMENT_SRRI_H

#include <vector>
#include <array>
#include "cache.h"
#include "modules.h"

constexpr long NUM_SET = 2048;  // Number of sets
constexpr long NUM_WAY = 16;    // Associativity (adjustable)
constexpr long HHT_ENTRIES = NUM_SET * NUM_WAY;  // Total cache blocks

constexpr long HISTORY_LENGTH = 4;  // Number of past hit counts stored

class srri : public champsim::modules::replacement
{

private: 

  struct RRIEntry {
        bool valid = false;
        champsim::address tag{};
       // std::array<uint64_t, HISTORY_LENGTH> rri_history = {0};
      // std::array<std::array<uint64_t, 10>, 4> rri_history = {{{0}}};
       std::vector<std::vector<uint64_t>> rri_history;
        uint64_t last_access_cycle = 0;
    };

   std::vector<std::vector<RRIEntry>> rri_table;  // Per-set RRI entries
   uint64_t global_cycle = 0;
   float predict_rri(const std::vector<std::vector<uint64_t>>& rri_history) const;

public:
  explicit srri(CACHE* cache);
  srri(CACHE* cache, long sets, long ways);

  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit);
};

#endif // REPLACEMENT_SRRI_H
