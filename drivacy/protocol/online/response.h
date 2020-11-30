// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#ifndef DRIVACY_PROTOCOL_ONLINE_RESPONSE_H_
#define DRIVACY_PROTOCOL_ONLINE_RESPONSE_H_

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace response {

types::Response ProcessResponse(const types::Response &response,
                                const types::QueryState &preshare);

}  // namespace response
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_RESPONSE_H_
