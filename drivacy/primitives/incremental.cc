// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#include "drivacy/primitives/incremental.h"

#include <iostream>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

namespace {

// multiplicative modulo inverse using extended Euclid algorithm.

void GcdExtended(uint32_t a, uint32_t b, int64_t *x, int64_t *y) {
  // Base Case
  if (a == 0) {
    *x = 0;
    *y = 1;
    return;
  }

  int64_t x1, y1;
  GcdExtended(b % a, a, &x1, &y1);
  *x = y1 - (b / a) * x1;
  *y = x1;
}

// Function to find modulo inverse of a
uint32_t ModInverse(uint32_t a, uint32_t m) {
  int64_t x, y;
  GcdExtended(a, m, &x, &y);
  return (x < 0 ? x + m : x) % m;
}

}  // namespace

std::vector<types::IncrementalSecretShare> PreIncrementalSecretShares(
    uint32_t numparty) {
  std::vector<types::IncrementalSecretShare> shares;
  for (uint32_t i = 0; i < numparty; i++) {
    uint32_t x = util::Rand32(0, util::Prime());
    uint32_t y = util::Rand32(1, util::Prime());
    shares.push_back({x, y});
  }
  return shares;
}

uint32_t GenerateIncrementalSecretShares(
    uint32_t query,
    const std::vector<types::IncrementalSecretShare> &preshares) {
  uint64_t t = query;
  for (uint32_t i = preshares.size(); i > 0; i--) {
    const types::IncrementalSecretShare &share = preshares.at(i - 1);
    uint64_t y_inv = ModInverse(share.y, util::Prime());
    t += (t < share.x ? util::Prime() : 0);
    t -= share.x;
    t = (t * y_inv) % util::Prime();
  }
  return t;
}

uint32_t IncrementalReconstruct(uint64_t tally,
                                const types::IncrementalSecretShare &share) {
  return (tally * share.y + share.x) % util::Prime();
}

}  // namespace primitives
}  // namespace drivacy
