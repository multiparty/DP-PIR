// Copyright 2020 multiparty.org

// Additive  secret sharing scheme.

#include "drivacy/primitives/additive.h"

#include <cstdlib>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

std::vector<uint64_t> GenerateAdditiveSecretShares(uint64_t query,
                                                   uint64_t numparty) {
  std::vector<uint64_t> shares;

  uint64_t t = 0;
  for (size_t i = 0; i < numparty - 1; i++) {
    uint64_t x = std::rand() % util::Prime();
    t = (t + x) % util::Prime();
    shares.push_back(x);
  }
  uint64_t last_x = util::Mod(query - t, util::Prime());
  shares.push_back(last_x);

  return shares;
}

uint64_t AdditiveReconstruct(uint64_t tally, uint64_t share) {
  return (tally + share) % util::Prime();
}

}  // namespace primitives
}  // namespace drivacy
