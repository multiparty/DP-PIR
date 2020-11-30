// Copyright 2020 multiparty.org

// This file contains backend-specific portions of the protocol.
// The backend still executes the appropriate code from query and response
// on top of this code.

#ifndef DRIVACY_PROTOCOL_ONLINE_BACKEND_H_
#define DRIVACY_PROTOCOL_ONLINE_BACKEND_H_

#include <unordered_map>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace backend {

types::Response QueryToResponse(const types::Query &query,
                                const types::CommonReferenceMap &common_map,
                                const types::Table &table);

}  // namespace backend
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_BACKEND_H_
