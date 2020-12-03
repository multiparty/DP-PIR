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

// Sample the noise histogram to figure out how many common references
// we need.
std::vector<uint32_t> SampleNoiseHistogram(uint32_t machine_id,
                                           uint32_t parallelism,
                                           uint32_t table_size, uint32_t span,
                                           uint32_t cutoff);

// Create common references for the given count.
std::vector<std::vector<types::Message>> CommonReferenceForNoise(
    uint32_t party_id, uint32_t party_count, uint32_t count, uint32_t seed);

}  // namespace noise
}  // namespace offline
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_OFFLINE_NOISE_H_
