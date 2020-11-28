// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#ifndef DRIVACY_PROTOCOL_NOISE_H_
#define DRIVACY_PROTOCOL_NOISE_H_

#include <utility>
#include <vector>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace noise {

std::pair<uint32_t, std::vector<uint32_t>> SampleNoise(uint32_t machine_id,
                                                       uint32_t parallelism,
                                                       uint32_t table_size,
                                                       double span,
                                                       double cutoff);

std::vector<types::OutgoingQuery> MakeNoisyQueries(
    uint32_t party_id, uint32_t machine_id, const types::Configuration &config,
    const types::Table &table, const std::vector<uint32_t> &counts);

}  // namespace noise
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_NOISE_H_
