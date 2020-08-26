// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#include "drivacy/primitives/secret_sharing.h"

#include <cstdlib>

namespace drivacy {
namespace primitives {

uint64_t PRIME = 0xffffffffffffffff - 58;  // 2^64 - 59 is prime.

namespace {

inline uint64_t Mod(uint64_t a, uint64_t modulus) {
  return a % modulus > 0 ? a : a + modulus;
}

}  // namespace

std::vector<IncrementalSecretShare> GenerateIncrementalSecretShares(
    uint64_t query, uint32_t numparty) {
  std::vector<IncrementalSecretShare> shares;

  uint64_t t = 1;
  for (uint32_t i = 0; i < numparty - 1; i++) {
    uint64_t x = std::rand() % PRIME;
    uint64_t y = std::rand() % PRIME;
    t = (t * y + x) % PRIME;
    shares.push_back({x, y});
  }
  uint64_t last_y = std::rand() % PRIME;
  uint64_t last_x = Mod(query - t * last_y, PRIME);
  shares.push_back({last_x, last_y});

  return shares;
}

uint64_t IncrementalReconstruct(uint64_t tally, IncrementalSecretShare share) {
  return (share.y * tally + share.x) % PRIME;
}

}  // namespace primitives
}  // namespace drivacy
