// Copyright 2020 multiparty.org

// Additive  secret sharing scheme.

#include "drivacy/primitives/additive_secret_sharing.h"

#include <cstdlib>

namespace drivacy {
namespace primitives {

uint64_t PRIME = 0xffffffffffffffff - 58;  // 2^64 - 59 is prime.

namespace {

inline uint64_t Mod(uint64_t a, uint64_t modulus) {
  return a % modulus > 0 ? a : a + modulus;
}

}  // namespace

std::vector<uint64_t> GenerateAdditiveSecretShares(
    uint64_t query, uint64_t numparty) {
  std::vector<uint64_t> shares;

  uint64_t t = 0;
  for (size_t i = 0; i < numparty - 1; i++) {
    uint64_t x = std::rand() % PRIME;
    t = t  + x % PRIME;
    shares.push_back(x);
  }
  uint64_t last_x = Mod(query - t, PRIME);
  shares.push_back(last_x);

  return shares;
}

uint64_t AdditiveReconstruct(uint64_t tally, uint64_t share) {
  return (tally + share) % PRIME;
}

}  // namespace primitives
}  // namespace drivacy
