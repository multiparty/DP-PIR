// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#ifndef DRIVACY_PRIMITIVES_ADDITIVE_H_
#define DRIVACY_PRIMITIVES_ADDITIVE_H_

#include <cstdint>
#include <vector>

namespace drivacy {
namespace primitives {

// Additively secret share the given query into numparty-many shares.
std::vector<uint32_t> GenerateAdditiveSecretShares(uint32_t numparty);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "GenerateAdditiveSecretShares(query, n)", in order, such that the
// resulting tally of every call is fed to the next, and the tally was initially
// 1, then the final tally will be equal to query (complete reconstruction).
uint32_t AdditiveReconstruct(uint32_t tally, uint32_t share);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_ADDITIVE_H_
