// Copyright 2020 multiparty.org

// Additive  secret sharing scheme.

#include "drivacy/primitives/additive.h"

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

std::vector<uint32_t> GenerateAdditiveSecretShares(uint32_t numparty) {
  std::vector<uint32_t> shares;

  uint32_t t = 0;
  for (size_t i = 0; i < numparty - 1; i++) {
    uint32_t x = util::Rand32(0, util::Prime());
    t = (t + x) % util::Prime();
    shares.push_back(x);
  }
  uint64_t last_x = util::Prime() - t;
  shares.push_back(last_x);

  return shares;
}

uint32_t AdditiveReconstruct(uint32_t tally, uint32_t share) {
  return (tally + share) % util::Prime();
}

}  // namespace primitives
}  // namespace drivacy
