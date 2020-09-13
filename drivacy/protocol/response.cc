// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/response.h"

#include <cstdint>

#include "drivacy/primitives/additive.h"

namespace drivacy {
namespace protocol {
namespace response {

types::Response ProcessResponse(const types::Response &response,
                                uint64_t preshare) {
  // Find next tally.
  uint64_t tally = primitives::AdditiveReconstruct(response.tally(), preshare);
  return types::Response(tally);
}

}  // namespace response
}  // namespace protocol
}  // namespace drivacy
