// Copyright 2020 multiparty.org

// Our Incremental and non-malleable secret sharing scheme.

#ifndef DRIVACY_PRIMITIVES_SECRET_SHARING_H_
#define DRIVACY_PRIMITIVES_SECRET_SHARING_H_

#include <cstdint>

namespace drivacy {
namespace primitives {

extern uint64_t PRIME;

uint64_t** IncrementalSecretShare(uint64_t query, uint64_t numparty);
uint64_t IncrementalReconstruct(uint64_t** shares, uint64_t numparty);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_SECRET_SHARING_H_
