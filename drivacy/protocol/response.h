// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#ifndef DRIVACY_PROTOCOL_RESPONSE_H_
#define DRIVACY_PROTOCOL_RESPONSE_H_

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace response {

types::Response ProcessResponse(const types::Response &response,
                                uint64_t preshare);

}  // namespace response
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_RESPONSE_H_
