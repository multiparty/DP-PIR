// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/online/response.h"

#include <cstdint>

#include "drivacy/primitives/additive.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace response {

types::Response ProcessResponse(const types::Response &response,
                                const types::QueryState &preshare) {
  // Find next tally.
  return primitives::AdditiveReconstruct(response, preshare);
}

}  // namespace response
}  // namespace online
}  // namespace protocol
}  // namespace drivacy
