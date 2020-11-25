// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#ifndef DRIVACY_PROTOCOL_NOISE_H_
#define DRIVACY_PROTOCOL_NOISE_H_

#include <list>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace noise {

std::list<types::OutgoingQuery> SampleNoise(uint32_t party_id,
                                            uint32_t machine_id,
                                            const types::Configuration &config,
                                            const types::Table &table);

}  // namespace noise
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_NOISE_H_
