// Copyright 2020 multiparty.org

// This file contains backend-specific portions of the protocol.
// The backend still executes the appropriate code from query and response
// on top of this code.

#ifndef DRIVACY_PROTOCOL_BACKEND_H_
#define DRIVACY_PROTOCOL_BACKEND_H_

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace backend {

types::Response QueryToResponse(const types::IncomingQuery &query,
                                const types::Configuration &config,
                                const types::Table &table);

}  // namespace backend
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_BACKEND_H_
