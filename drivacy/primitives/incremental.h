// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#ifndef DRIVACY_PRIMITIVES_INCREMENTAL_H_
#define DRIVACY_PRIMITIVES_INCREMENTAL_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace primitives {

// Generates random preshares of an value that is yet to be determined.
std::vector<types::IncrementalSecretShare> PreIncrementalSecretShares(
    uint32_t numparty);

// Compute the tally to secret share query using the given preshares.
uint32_t GenerateIncrementalSecretShares(
    uint32_t query,
    const std::vector<types::IncrementalSecretShare> &preshares);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "PreIncrementalSecretShares(n)" and the tally returned by
// "GenerateIncrementalSecretShares(query, shares)", such that the
// resulting tally of every call is fed to the next, and the tally was initially
// 1, then the final tally will be equal to query (complete reconstruction).
uint32_t IncrementalReconstruct(uint64_t tally,
                                const types::IncrementalSecretShare &share);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_INCREMENTAL_H_
