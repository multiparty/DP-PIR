// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#ifndef DRIVACY_PRIMITIVES_ADDITIVE_SECRET_SHARING_H_
#define DRIVACY_PRIMITIVES_ADDITIVE_SSECRET_SHARING_H_

#include <cstdint>
#include <vector>

namespace drivacy {
namespace primitives {

extern uint64_t PRIME;


// Additively secret share the given query into numparty-many shares.
std::vector<uint64_t> GenerateAdditiveSecretShares(
    uint64_t query, uint64_t numparty);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "GenerateAdditiveSecretShares(query, n)", in order, such that the
// resulting tally of every call is fed to the next, and the tally was initially
// 1, then the final tally will be equal to query (complete reconstruction).
uint64_t AdditiveReconstruct(uint64_t tally, uint64_t share);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_SECRET_SHARING_H_
