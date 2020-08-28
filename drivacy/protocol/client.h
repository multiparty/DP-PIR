// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#ifndef DRIVACY_PROTOCOL_CLIENT_H_
#define DRIVACY_PROTOCOL_CLIENT_H_

#include <cstdint>
#include <utility>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/messages.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace client {

types::Query CreateQuery(uint64_t value, const types::Configuration &config,
                         types::ClientState *state);

std::pair<uint64_t, uint64_t> ReconstructResponse(
    const types::Response &response, types::ClientState *state);

}  // namespace client
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_CLIENT_H_
