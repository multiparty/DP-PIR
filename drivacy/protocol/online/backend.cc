// Copyright 2020 multiparty.org

// This file contains backend-specific portions of the protocol.
// The backend still executes the appropriate code from query and response
// on top of this code.

#include "drivacy/protocol/online/backend.h"

#include <cstdint>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace backend {

types::Response QueryToResponse(const types::Query &query,
                                const types::CommonReferenceMap &common_map,
                                const types::Table &table) {
  // Find corresponding common reference shares.
  types::Tag tag = query.tag;
  const types::CommonReference &common = common_map.at(tag);

  // Reconstruct the query, and find the response.
  uint32_t tally = query.tally;
  uint32_t query_value =
      primitives::IncrementalReconstruct(tally, common.incremental_share);
  uint32_t response_value = table.at(query_value);

  // Share response value using preshare from query.
  types::Response response_tally =
      primitives::AdditiveReconstruct(response_value, common.preshare);

  return response_tally;
}

}  // namespace backend
}  // namespace online
}  // namespace protocol
}  // namespace drivacy
