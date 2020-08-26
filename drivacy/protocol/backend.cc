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
                                const types::Table &table, uint32_t parties) {
  const types::QueryShare &share = query.shares(0);
  uint64_t query_value =
      primitives::IncrementalReconstruct(query.tally(), {share.x(), share.y()});
  uint64_t response_value = table.at(query_value);

  // Debugging.
  std::cout << "query: " << query_value << std::endl;
  std::cout << "response: " << response_value << std::endl;

  // Share response value.
  auto shares =
      primitives::GenerateAdditiveSecretShares(response_value, parties);

  // Construct response object.
  types::Response response;
  response.set_tag(query.tag());
  response.set_tally(0);
  for (const auto &share : shares) {
    response.add_shares(share);
  }
  return response;
}

}  // namespace backend
}  // namespace protocol
}  // namespace drivacy
