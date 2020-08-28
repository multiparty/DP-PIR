// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#ifndef DRIVACY_PROTOCOL_CLIENT_H_
#define DRIVACY_PROTOCOL_CLIENT_H_

#include <cstdint>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/messages.pb.h"

namespace drivacy {
namespace protocol {
namespace client {

types::Query CreateQuery(uint64_t value, const types::Configuration &config);
void ReconstructResponse(const types::Response &response);

}  // namespace client
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_CLIENT_H_
