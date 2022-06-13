#include "DPPIR/sharing/incremental.h"

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace sharing {

namespace {

// multiplicative modulo inverse using extended Euclid algorithm.
void GcdExtended(uint32_t a, uint32_t b, int64_t* x, int64_t* y) {
  // Base Case
  if (a == 0) {
    *x = 0;
    *y = 1;
    return;
  }
  GcdExtended(b % a, a, y, x);
  *x = *x - (b / a) * *y;
}

// Function to find modulo inverse of a
uint32_t ModInverse(uint32_t a, uint32_t m) {
  int64_t x, y;
  GcdExtended(a, m, &x, &y);
  return (x < 0 ? x + m : x) % m;
}

}  // namespace

std::vector<incremental_share_t> PreIncrementalSecretShares(size_t n) {
  std::vector<incremental_share_t> shares;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t x = randombytes_uniform(INCREMENTAL_PRIME);
    uint32_t y = randombytes_uniform(INCREMENTAL_PRIME - 1) + 1;
    shares.push_back({x, y});
  }
  return shares;
}

incremental_tally_t GenerateIncrementalTally(
    key_t query, const std::vector<incremental_share_t>& preshares) {
  // Turn query to uint64_t so operations with x and y fit.
  uint64_t t = query;
  for (size_t i = preshares.size(); i > 0; i--) {
    const auto& share = preshares.at(i - 1);
    t = t + (t < share.x ? INCREMENTAL_PRIME : 0) - share.x;
    t = (t * ModInverse(share.y, INCREMENTAL_PRIME)) % INCREMENTAL_PRIME;
  }
  return t;
}

incremental_tally_t IncrementalReconstruct(incremental_tally_t tally,
                                           const incremental_share_t& share) {
  return (static_cast<uint64_t>(tally) * share.y + share.x) % INCREMENTAL_PRIME;
}

}  // namespace sharing
}  // namespace DPPIR
