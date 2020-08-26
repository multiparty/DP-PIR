// Copyright 2020 multiparty.org

// This file contains backend-specific portions of the protocol.
// The backend still executes the appropriate code from query and response
// on top of this code.

#include "drivacy/protocol/backend.h"

#include <iostream>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace backend {

types::Response QueryToResponse(const types::Query &query,
                                const types::Table &table) {
  const types::QueryShare &share = query.shares(0);
  uint64_t query_value =
      primitives::IncrementalReconstruct(query.tally(), {share.x(), share.y()});
  uint64_t response_value = table.at(query_value);

  // Debugging.
  std::cout << "\tbackend query: " << query_value << std::endl;
  std::cout << "\tbackend response: " << response_value << std::endl;

  // Share response value using preshare from query.
  uint64_t response_tally =
      primitives::AdditiveReconstruct(response_value, query.preshares(0));

  // Construct response object.
  types::Response response;
  response.set_tag(query.tag());
  response.set_tally(response_tally);

  return response;
}

}  // namespace backend
}  // namespace protocol
}  // namespace drivacy
