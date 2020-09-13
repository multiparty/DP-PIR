// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#ifndef DRIVACY_PROTOCOL_CLIENT_H_
#define DRIVACY_PROTOCOL_CLIENT_H_

#include <cstdint>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace client {

types::OutgoingQuery CreateQuery(uint64_t value,
                                 const types::Configuration &config);

uint64_t ReconstructResponse(const types::Response &response,
                             uint64_t preshare);

}  // namespace client
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_CLIENT_H_
