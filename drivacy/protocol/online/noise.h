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

std::vector<types::Query> MakeNoisyQueries(
    uint32_t party_id, uint32_t machine_id, uint32_t parallelism,
    const types::Table &table, double span, double cutoff,
    const std::vector<std::vector<types::Message>> &commons);

}  // namespace noise
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_NOISE_H_
