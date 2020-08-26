// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#include "drivacy/primitives/incremental.h"

#include <cstdlib>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

std::vector<types::IncrementalSecretShare> GenerateIncrementalSecretShares(
    uint64_t query, uint32_t numparty) {
  std::vector<types::IncrementalSecretShare> shares;

  uint64_t t = 1;
  for (uint32_t i = 0; i < numparty - 1; i++) {
    uint64_t x = std::rand() % util::Prime();
    uint64_t y = std::rand() % util::Prime();
    t = (t * y + x) % util::Prime();
    shares.push_back({x, y});
  }
  uint64_t last_y = std::rand() % util::Prime();
  uint64_t last_x = util::Mod(query - t * last_y, util::Prime());
  shares.push_back({last_x, last_y});

  return shares;
}

uint64_t IncrementalReconstruct(uint64_t tally,
                                types::IncrementalSecretShare share) {
  return (share.y * tally + share.x) % util::Prime();
}

}  // namespace primitives
}  // namespace drivacy
