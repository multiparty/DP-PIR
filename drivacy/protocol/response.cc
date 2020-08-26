// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/response.h"

#include "drivacy/primitives/additive.h"

namespace drivacy {
namespace protocol {
namespace response {

types::Response ProcessResponse(const types::Response &response) {
  // Find next tally.
  uint64_t share = response.shares(0);
  uint64_t next_tally =
      primitives::AdditiveReconstruct(response.tally(), share);

  // Construct next response object.
  types::Response next_response;
  next_response.set_tag(response.tag());
  next_response.set_tally(next_tally);
  auto iterator = response.shares().cbegin();
  for (iterator++; iterator != response.shares().cend(); iterator++) {
    next_response.add_shares(*iterator);
  }

  return next_response;
}

}  // namespace response
}  // namespace protocol
}  // namespace drivacy
