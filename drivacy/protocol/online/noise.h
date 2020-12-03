// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#ifndef DRIVACY_PROTOCOL_ONLINE_NOISE_H_
#define DRIVACY_PROTOCOL_ONLINE_NOISE_H_

#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace noise {

// TODO(babman): after reading from a file, this should be removed from
// online.
std::vector<uint32_t> SampleNoiseHistogram(uint32_t machine_id,
                                           uint32_t parallelism,
                                           uint32_t table_size, uint32_t span,
                                           uint32_t cutoff);

std::vector<types::Query> MakeNoiseQueriesFromHistogram(
    uint32_t party_id, uint32_t machine_id, uint32_t party_count,
    uint32_t parallelism, const types::Table &table,
    const std::vector<uint32_t> &noise_histogram,
    const std::vector<std::vector<types::Message>> &commons);

}  // namespace noise
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_NOISE_H_
