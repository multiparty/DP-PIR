// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#ifndef DRIVACY_PROTOCOL_ONLINE_CLIENT_H_
#define DRIVACY_PROTOCOL_ONLINE_CLIENT_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace client {

// Equivalent to CreateQuery(value, config, 0);
types::Query CreateQuery(uint64_t value,
                         const std::vector<types::Message> &common);

// Creates a query meant to be handled by the system starting from
// party_id + 1.
types::Query CreateQuery(uint64_t value,
                         const std::vector<types::Message> &common,
                         uint32_t party_id);

uint64_t ReconstructResponse(const types::Response &response,
                             const types::QueryState &preshare);

}  // namespace client
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_CLIENT_H_
