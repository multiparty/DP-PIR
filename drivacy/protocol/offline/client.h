// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#ifndef DRIVACY_PROTOCOL_OFFLINE_CLIENT_H_
#define DRIVACY_PROTOCOL_OFFLINE_CLIENT_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace offline {
namespace client {

// Sample a random common reference tuple for each party.
std::vector<types::Message> SampleCommonReference(uint32_t seed,
                                                  uint32_t party_id,
                                                  uint32_t party_count);

}  // namespace client
}  // namespace offline
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_OFFLINE_CLIENT_H_
