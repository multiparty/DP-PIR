// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#ifndef DRIVACY_PRIMITIVES_INCREMENTAL_H_
#define DRIVACY_PRIMITIVES_INCREMENTAL_H_

#include <cstdint>
#include <vector>

namespace drivacy {
namespace primitives {

struct IncrementalSecretShare {
  uint64_t x;  // The additive component.
  uint64_t y;  // The multiplicative component.
};

// Secret share the given query into numparty-many IncrementalSecretShares.
std::vector<IncrementalSecretShare> GenerateIncrementalSecretShares(
    uint64_t query, uint32_t numparty);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "GenerateIncrementalSecretShares(query, n)", in order, such that the
// resulting tally of every call is fed to the next, and the tally was initially
// 1, then the final tally will be equal to query (complete reconstruction).
uint64_t IncrementalReconstruct(uint64_t tally, IncrementalSecretShare share);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_INCREMENTAL_H_
