// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#ifndef DRIVACY_PROTOCOL_OFFLINE_NOISE_H_
#define DRIVACY_PROTOCOL_OFFLINE_NOISE_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace offline {
namespace noise {

// Upper bound on how much noise we might generate.
uint64_t UpperBound(uint32_t machine_id, uint32_t parallelism,
                    uint32_t table_size, double span, double cutoff);

std::vector<std::vector<types::Message>> CommonReferenceForNoise(
    uint32_t party_id, uint32_t party_count, uint32_t count, uint32_t seed);

}  // namespace noise
}  // namespace offline
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_OFFLINE_NOISE_H_
